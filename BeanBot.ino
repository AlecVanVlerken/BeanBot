#include <Arduino.h>
#include "src/beanbot/Config.h"
#include "src/beanbot/Types.h"
#include "src/vendor/Adafruit_PWMServoDriver/Adafruit_PWMServoDriver.h"
#include "src/vendor/HX711/HX711.h"
#include "src/vendor/LiquidCrystal/LiquidCrystal.h"

// Existing communication and workflow remain procedural until their own stages.
bool withWifi = Config::Communication::useWifi;
String startsequence = Config::Communication::commandStart;
String stopsequence = Config::Communication::commandEnd;
String startsequenceSetup = Config::Communication::setupStart;
String stopsequenceSetup = Config::Communication::setupEnd;

Adafruit_PWMServoDriver MijnServo;
HX711 scale;
LiquidCrystal lcd(Config::Display::rs, Config::Display::enablePin,
                  Config::Display::d4, Config::Display::d5,
                  Config::Display::d6, Config::Display::d7);

beanbot::Order order = Config::initialOrder;
beanbot::Stock stock = {
  {0, Config::Display::whitePosition},
  {0, Config::Display::blackPosition},
  {0, Config::Display::redPosition}
};
beanbot::CollectionProgress collection = {0, 0, 0, beanbot::BeanColor::None};
beanbot::ScanProgress scan = {0, 0, 0, 0, true};
beanbot::ColorReading colorReading = {0, 0, 0};

bool start_metingen = false;
bool start_bot = false;
bool kleursensor_aan = false;
bool code_nog_niet_doorlopen = true;

// Mechanism state moves into its owning objects in stage 2.
bool servo_wagen_wijzerszin = true;
bool dc_motor_aan = false;
bool dc_motor_wijzerszin = false;
bool servo_bak_wijzerszin = false;
bool servo_afstandssensor_terug = false;
int interval_afstand_servo = Config::Servo::firstProbeStep;

void setup() {
  // --> Wifi setup
  Serial2.begin(Config::Communication::baud); // Serial2 is de communicatie met de ESP32 (wifi-module).
  pinMode(Config::Communication::shieldPin, OUTPUT);
  digitalWrite(Config::Communication::shieldPin,HIGH);

  // ---> Colorsensor setup
  // Stel de pin-modi voor de kleursensor in
  pinMode(Config::ColorSensor::s0, OUTPUT);
  pinMode(Config::ColorSensor::s1, OUTPUT);
  pinMode(Config::ColorSensor::s2, OUTPUT);
  pinMode(Config::ColorSensor::s3, OUTPUT);
  pinMode(Config::ColorSensor::outputPin, INPUT);

  pinMode(Config::ColorSensor::enablePin, OUTPUT);
  digitalWrite(Config::ColorSensor::enablePin, LOW);

  // Stel de frequentieschaling in op 20%
  digitalWrite(Config::ColorSensor::s0, HIGH);
  digitalWrite(Config::ColorSensor::s1, LOW);

  // ---> DC motor setup
  pinMode(Config::Conveyor::enablePin,OUTPUT) ; //Logische pinnen worden ook ingesteld als uitvoer
  pinMode(Config::Conveyor::reversePin,OUTPUT) ;

  // ---> Servo setup
  // -----------------NIET AANPASSEN ------------------------
  MijnServo.begin();
  MijnServo.setOscillatorFrequency(Config::Servo::oscillatorHz);
  MijnServo.setPWMFreq(Config::Servo::frequencyHz);

  // ---> Stappenmotor setup
  // Stelt drie twee pinnen in als Uitgangen
  pinMode(Config::Frame::stepPin,OUTPUT);
  pinMode(Config::Frame::directionPin,OUTPUT);
  pinMode(Config::Frame::enablePin,OUTPUT);
  digitalWrite(Config::Frame::enablePin,LOW); // Schakelt de motor in

  // ---> Gewichtssensor setup
  scale.begin(Config::Scale::dataPin, Config::Scale::clockPin);
  scale.set_scale(Config::Scale::calibrationFactor); // Deze waarde wordt verkregen met behulp van de SparkFun_HX711_Calibration-schets
  scale.tare(); // Ervan uitgaande dat er bij het opstarten geen gewicht op de weegschaal staat, zet u de weegschaal terug op 0

  // ---> LCD setup
  // Stel het aantal kolommen en rijen van het LCD-scherm in
  lcd.begin(Config::Display::columns, Config::Display::rows);
}


// ---------------------------------- Functies die gebruik maken van de wifi om het process in gang te steken ----------------------------------

void start_zonder_wifi() {
  withWifi = false;
  collection.previousWeight = 0;
  collection.currentWeight = 0;
  collection.selectedColor = beanbot::BeanColor::None;
  code_nog_niet_doorlopen = true;

  // Kies hier het nodig gewicht van bonen
  order = Config::demoOrder;

  stappen_motor_grote_stap_links();
  start_metingen = true;
  kleursensor_aan = true;
}


void wifi() {
  // Kijk steeds opnieuw of er iets werd verzonden over wifi.
  String command = checkwifi(); //Hier voer je de functie checkwifi() uit en sla je het resultaat op in de string command. De functie checkwifi() wordt lager in dit document gecodeerd.
  // Voer het commando uit als er iets werd ontvangen
  if (command.length()>0){
    //Splits het command op in commando/parameters
    String witte = command.substring(0,command.indexOf(':'));
    String zwarte = command.substring(command.indexOf(':')+1,command.lastIndexOf(':'));
    String rode = command.substring(command.lastIndexOf(':')+1,command.length());

    order.whiteGrams = atol(witte.c_str());
    order.blackGrams = atol(zwarte.c_str());
    order.redGrams = atol(rode.c_str());

    withWifi = false;
    code_nog_niet_doorlopen = true;
    collection.previousWeight = 0;
    collection.currentWeight = 0;
    collection.selectedColor = beanbot::BeanColor::None;
    scan.currentDistance = 0;

    stappen_motor_grote_stap_links();
    start_metingen = true;
    kleursensor_aan = true;
  }
  if(withWifi){ // Als je wifi aanstaat, maakt dit if-statement het mogelijk om van de seriële monitor naar de app te communiceren. Je typt iets willekeurigs in de seriële monitor, en de app toont deze boodschap.
    if(Serial.available()){ // Voer deze actie uit als er iets verscheen in de seriële monitor.
      String boodschap=Serial.readString(); // Lees de boodschap in die je typte en sla deze op als een string.
      fromMonitorToApp(boodschap); // De void fromMonitorToApp() wordt onderaan dit bestand gedefinieerd.
    }
  }
}


// ---------------------------------------------------- Functies voor de connectie met ESP32; NIET AANPASSEN AUB ---------------------------------------------------------

String checkwifi() {
  //Kijkt of er iets werd verzonden over wifi, ontvangt het en decodeert het ook.
  String command = "";

  if (withWifi){
    // Je werkt met wifi, dus je leest de input van de ESP32.
    while (Serial2.available())
    {
      command = Serial2.readString();
    }
    // Haal de start- en stopsequence van het command
    if (command.indexOf(startsequence)>0){
      command = command.substring(command.indexOf(startsequence)+startsequence.length());
      command = command.substring(0,command.indexOf(stopsequence));
    }
    // Is het een setup commando?
    else if (command.indexOf(startsequenceSetup)>0){
      displayESP32Setup(command);
      command = "";
    }
    else {
      command = "";
    }
  }
  else{
  // Je werkt zonder wifi, dus je leest de input van de seriële monitor.
     while (Serial.available())
    {
      command = Serial.readString();
    }
  }
  return command;
}

void sendWifi(String message){
  if (withWifi) {
    Serial2.println(message);
  }
  else {
    Serial.print("Message for Wifi: ");
    Serial.println(message);
  }
}

void displayESP32Setup(String command){
  command = command.substring(command.indexOf(startsequenceSetup)+startsequenceSetup.length());
  command = command.substring(0,command.indexOf(stopsequenceSetup));
  Serial.println(command);
}

void fromMonitorToApp(String message){
  sendWifi(message);
}

// ---------------------------------- PAS HIERONDER AAN WELKE COMMANDO'S JE ZELF WILT ONTVANGEN --------------------------------------------------------

// ---------------------------------- Functies voor servo's ----------------------------------

// Functie om de wagen heen en weer te laten gaan
void servo_wagen() {
  if (servo_wagen_wijzerszin) {
    // naar voor: 0 en na achteren 180
    int servoPWM = Config::Servo::carriageForward;
    servoPWM = map(servoPWM, Config::Servo::minimumAngle, Config::Servo::standardRange, Config::Servo::pulseMin, Config::Servo::pulseMax);
    MijnServo.setPWM(Config::Servo::lastChannel-Config::Servo::carriageConnector, 0, servoPWM); // de "15-" is een interne correctie, niet verwijderen!!
    servo_wagen_wijzerszin = false;
  } else {
    int servoPWM = Config::Servo::carriageBack;
    servoPWM = map(servoPWM, Config::Servo::minimumAngle, Config::Servo::standardRange, Config::Servo::pulseMin, Config::Servo::pulseMax);
    MijnServo.setPWM(Config::Servo::lastChannel-Config::Servo::carriageConnector, 0, servoPWM); // de "15-" is een interne correctie, niet verwijderen!!
    servo_wagen_wijzerszin = true;
  }
    delay(Config::Servo::travelDelayMs);
}


// Functie om de bak met de bonen derin te openen
void servo_bak() {
  // toe = 145, open = 65
  if (servo_bak_wijzerszin) {
    int servoPWM = Config::Servo::gateClosed;
    servoPWM = map(servoPWM, Config::Servo::minimumAngle, Config::Servo::standardRange, Config::Servo::pulseMin, Config::Servo::pulseMax);
    MijnServo.setPWM(Config::Servo::lastChannel-Config::Servo::gateConnector, 0, servoPWM); // de "15-" is een interne correctie, niet verwijderen!!
    servo_bak_wijzerszin = false;
  } else {
    int servoPWM = Config::Servo::gateOpen;
    servoPWM = map(servoPWM, Config::Servo::minimumAngle, Config::Servo::standardRange, Config::Servo::pulseMin, Config::Servo::pulseMax);
    MijnServo.setPWM(Config::Servo::lastChannel-Config::Servo::gateConnector, 0, servoPWM); // de "15-" is een interne correctie, niet verwijderen!!
    servo_bak_wijzerszin = true;
  }
    delay(Config::Servo::travelDelayMs);
}


// Functie om de afstandsensor naar beneden en naar boven te laten bewegen
void servo_afstandssensor() {
  // afstand boven:210 en beneden:0
  if (servo_afstandssensor_terug) {
    int servoPWM = Config::Servo::probeTop;
    servoPWM = map(servoPWM, Config::Servo::minimumAngle, Config::Servo::probeRange, Config::Servo::pulseMin, Config::Servo::pulseMax);
    MijnServo.setPWM(Config::Servo::lastChannel-Config::Servo::probeConnector, 0, servoPWM); // de "15-" is een interne correctie, niet verwijderen!!
  } else {
    int servoPWM = Config::Servo::probeTop - interval_afstand_servo;
    servoPWM = map(servoPWM, Config::Servo::minimumAngle, Config::Servo::probeRange, Config::Servo::pulseMin, Config::Servo::pulseMax);
    MijnServo.setPWM(Config::Servo::lastChannel-Config::Servo::probeConnector, 0, servoPWM); // de "15-" is een interne correctie, niet verwijderen!!
  }

  interval_afstand_servo += Config::Servo::probeStep;
  delayMicroseconds(Config::Servo::probeDelayUs);
}


// ---------------------------------- Functies voor de stappenmotor ----------------------------------

// Functie draait de constructie naar rechts volgens de kleinere hoek, hier 15°
void stappen_motor_kleine_stap_rechts() {
  digitalWrite(Config::Frame::directionPin,HIGH); // Hiermee kan de motor in een bepaalde richting bewegen
  // Maakt pulsen met elke puls gelijk aan 1.8° zodat 200 pulsen gelijk is aan één volledige cyclusomwenteling, 360°
  for(int x = 0; x < Config::Frame::smallSteps; x++) {
    digitalWrite(Config::Frame::stepPin,HIGH);
    delay(Config::Frame::pulseDelayMs);    // door deze tijdsvertraging tussen de stappen te wijzigen, kunnen we de rotatiesnelheid wijzigen
    digitalWrite(Config::Frame::stepPin,LOW);
    delay(Config::Frame::pulseDelayMs);
  }
  delay(Config::Frame::settleDelayMs);
}


// Functie draait de constructie naar rechts volgens de grotere hoek, hier 30°
void stappen_motor_grote_stap_rechts() {
  digitalWrite(Config::Frame::directionPin,HIGH);
  for(int x = 0; x < Config::Frame::largeSteps; x++) {
    digitalWrite(Config::Frame::stepPin,HIGH);
    delay(Config::Frame::pulseDelayMs);
    digitalWrite(Config::Frame::stepPin,LOW);
    delay(Config::Frame::pulseDelayMs);
  }
  delay(Config::Frame::settleDelayMs);
}


// Functie draait de constructie naar links volgens de grotere hoek, hier 30°
void stappen_motor_grote_stap_links() {
  digitalWrite(Config::Frame::directionPin,LOW);
  for(int x = 0; x < Config::Frame::largeSteps; x++) {
    digitalWrite(Config::Frame::stepPin,HIGH);
    delay(Config::Frame::pulseDelayMs);
    digitalWrite(Config::Frame::stepPin,LOW);
    delay(Config::Frame::pulseDelayMs);
  }
  delay(Config::Frame::settleDelayMs);
}


// Functie draait de constructie naar links volgens de kleinere hoek, hier 15°
void stappen_motor_kleine_stap_links() {
  digitalWrite(Config::Frame::directionPin,LOW);
  for(int x = 0; x < Config::Frame::smallSteps; x++) {
    digitalWrite(Config::Frame::stepPin,HIGH);
    delay(Config::Frame::pulseDelayMs);
    digitalWrite(Config::Frame::stepPin,LOW);
    delay(Config::Frame::pulseDelayMs);
  }

  delay(Config::Frame::settleDelayMs);
}


// // ---------------------------------- Functies het berekenen van het aantal bonen met behulp van de afstandsensor ----------------------------------

// Functie om de afstand te verkrijgen van de afstandssensor
void afstandssensor() {
  long duration, cm;
  pinMode(Config::DistanceSensor::triggerPin, OUTPUT);
  digitalWrite(Config::DistanceSensor::triggerPin, LOW);
  delayMicroseconds(Config::DistanceSensor::triggerLowUs);
  digitalWrite(Config::DistanceSensor::triggerPin, HIGH);
  delayMicroseconds(Config::DistanceSensor::triggerHighUs);
  digitalWrite(Config::DistanceSensor::triggerPin, LOW);
  pinMode(Config::DistanceSensor::echoPin, INPUT);
  duration = pulseIn(Config::DistanceSensor::echoPin, HIGH);
  cm = microsecondsToCentimeters(duration);
  scan.currentDistance = cm;

  if (scan.firstDistance && cm != 0 ) {
    scan.referenceDistance = cm;
    scan.firstDistance = false;
  }
}


long microsecondsToCentimeters(long microseconds) {
   return microseconds / Config::DistanceSensor::microsecondsPerCentimeter / 2;
}


// Functie om de opslag bonen te bereken en wat er moet gebeuren na de berekening
void afstandssensor_berekeningen() {
  long tijd = 0;

  while (interval_afstand_servo <= Config::Servo::lastProbeStep) {
    afstandssensor();
    servo_afstandssensor();
    tijd += Config::Inventory::timeIncrement *(10^(-3));

    if (scan.currentDistance <= scan.referenceDistance) {
      long h = Config::Inventory::initialHeight - (60*Config::Inventory::rpm*Config::Inventory::radius*tijd)/(2*Config::Inventory::pi);
      if (h <= Config::Inventory::maximumHeight) {
        long V = (Config::Inventory::base*(h^2)*2)/(3*tan(Config::Inventory::angle));
      }
      if (h > Config::Inventory::maximumHeight) {
        long V = (Config::Inventory::base*(h^2)*2)/(3*tan(Config::Inventory::angle)) + Config::Inventory::base*Config::Inventory::length*(h - Config::Inventory::maximumHeight) + (6+ 2*((h - Config::Inventory::maximumHeight)^2)*Config::Inventory::pi*(h - Config::Inventory::maximumHeight))/6;
      }
      if (collection.selectedColor == beanbot::BeanColor::Red) {
        stock.red.weightGrams = (scan.volume/Config::Inventory::whiteRedBeanVolume)*Config::Inventory::whiteRedBeanWeight;
        collection.selectedColor = beanbot::BeanColor::None;
      }
      if (collection.selectedColor == beanbot::BeanColor::White) {
        stock.white.weightGrams = (scan.volume/Config::Inventory::whiteRedBeanVolume)*Config::Inventory::whiteRedBeanWeight;
        collection.selectedColor = beanbot::BeanColor::None;
      }
      if (collection.selectedColor == beanbot::BeanColor::Black) {
        stock.black.weightGrams = (scan.volume/Config::Inventory::blackBeanVolume)*Config::Inventory::blackBeanWeight;
        collection.selectedColor = beanbot::BeanColor::None;
      }
      servo_afstandssensor_terug = true;
      interval_afstand_servo = Config::Servo::firstProbeStep;
      break;
    }
  }

  servo_afstandssensor();
  servo_afstandssensor_terug = false;
  scan.reservoirsMeasured += 1;

  if (scan.reservoirsMeasured >= Config::reservoirCount) {
    start_metingen = false;
    scan.reservoirsMeasured = 0;
    stappen_motor_kleine_stap_links();
    stappen_motor_grote_stap_links();
    stappen_motor_grote_stap_links();
    start_bot = true;
  } else {
    stappen_motor_kleine_stap_rechts();
  }
  kleursensor_aan = true;
}


// ---------------------------------- Functie voor LCD ----------------------------------

void lcd_scherm() {
  // Clear the LCD
  lcd.clear();

  // Print the colors and their weights in the desired order
  if (stock.red.displayPosition == 1) {
    lcd.setCursor(0, 0);
    lcd.print("ROOD ");
    lcd.setCursor(0, 1);
    lcd.print(stock.red.weightGrams);
    lcd.print("g");
  }
  else if (stock.red.displayPosition == 2) {
    lcd.setCursor(6, 0);
    lcd.print("ROOD ");
    lcd.setCursor(6, 1);
    lcd.print(stock.red.weightGrams);
    lcd.print("g");
  }
  else if (stock.red.displayPosition == 3) {
    lcd.setCursor(12, 0);
    lcd.print("ROOD ");
    lcd.setCursor(12, 1);
    lcd.print(stock.red.weightGrams);
    lcd.print("g");
  }

  if (stock.white.displayPosition == 1) {
    lcd.setCursor(0, 0);
    lcd.print("WIT ");
    lcd.setCursor(0, 1);
    lcd.print(stock.white.weightGrams);
    lcd.print("g");
  }
  else if (stock.white.displayPosition == 2) {
    lcd.setCursor(6, 0);
    lcd.print("WIT ");
    lcd.setCursor(6, 1);
    lcd.print(stock.white.weightGrams);
    lcd.print("g");
  }
  else if (stock.white.displayPosition == 3) {
    lcd.setCursor(13, 0);
    lcd.print("WIT ");
    lcd.setCursor(12, 1);
    lcd.print(stock.white.weightGrams);
    lcd.print("g");
  }

  if (stock.black.displayPosition == 1) {
    lcd.setCursor(0, 0);
    lcd.print("ZWART ");
    lcd.setCursor(0, 1);
    lcd.print(stock.black.weightGrams);
    lcd.print("g");
  }
  else if (stock.black.displayPosition == 2) {
    lcd.setCursor(6, 0);
    lcd.print("ZWART ");
    lcd.setCursor(6, 1);
    lcd.print(stock.black.weightGrams);
    lcd.print("g");
  }
  else if (stock.black.displayPosition == 3) {
    lcd.setCursor(11, 0);
    lcd.print("ZWART ");
    lcd.setCursor(12, 1);
    lcd.print(stock.black.weightGrams);
    lcd.print("g");
  }
}


// ---------------------------------- Functie voor kleursensor en acties die doorgevoerd moeten worden bij bepaalde kleuren ----------------------------------

void kleursensor(){
  if (kleursensor_aan) {
    // Instellen van rood gefilterde fotodiodes om te lezen
    digitalWrite(Config::ColorSensor::s2,LOW);
    digitalWrite(Config::ColorSensor::s3,LOW);
    // Uitlezen van de uitgangsfrequentie
    colorReading.red = pulseIn(Config::ColorSensor::outputPin, LOW);
    delay(Config::ColorSensor::readingDelayMs);

    // Instellen van groen gefilterde fotodiodes om te lezen
    digitalWrite(Config::ColorSensor::s2,HIGH);
    digitalWrite(Config::ColorSensor::s3,HIGH);
    // Uitlezen van de uitgangsfrequentie
    colorReading.green = pulseIn(Config::ColorSensor::outputPin, LOW);
    delay(Config::ColorSensor::readingDelayMs);

    // Instellen van blauw gefilterde fotodiodes om te lezen
    digitalWrite(Config::ColorSensor::s2,LOW);
    digitalWrite(Config::ColorSensor::s3,HIGH);
    // Uitlezen van de uitgangsfrequentie
    colorReading.blue = pulseIn(Config::ColorSensor::outputPin, LOW);
    delay(Config::ColorSensor::readingDelayMs);

    // Aanpassaen van frequentie bereiken zodat de correcte bonen worden gededecteerd
    // Alle acties worden doorgevoerd na detectie van bonen
    if (Config::ColorSensor::redMaxWhite > colorReading.red) {
      kleursensor_aan = false;
      collection.selectedColor = beanbot::BeanColor::White;
      if (start_metingen) {
        stappen_motor_kleine_stap_rechts();
        scan.firstDistance = true;
        afstandssensor_berekeningen();
      } else {
        if (order.redGrams > 0) {
          servo_wagen();
          dc_motor_aan = true;
        } else {
          check_bakken();
        }
      }
    } else if (Config::ColorSensor::greenMinBlack < colorReading.green) {
      kleursensor_aan = false;
      collection.selectedColor = beanbot::BeanColor::Black;
      if (start_metingen) {
        stappen_motor_kleine_stap_rechts();
        scan.firstDistance = true;
        afstandssensor_berekeningen();
      } else {
        if (order.redGrams > 0) {
          servo_wagen();
          dc_motor_aan = true;
        } else {
          check_bakken();
        }
      }
    } else {
      kleursensor_aan = false;
      collection.selectedColor = beanbot::BeanColor::Red;
      if (start_metingen) {
        stappen_motor_kleine_stap_rechts();
        scan.firstDistance = true;
        afstandssensor_berekeningen();
      } else {
        if (order.redGrams > 0) {
          servo_wagen();
          dc_motor_aan = true;
        } else {
          check_bakken();
        }
      }
    }
  }
}


// ---------------------------------- Functies voor DC-motor, gewichtssensor en checken van bakken ----------------------------------

// --> DC motor functie
void dc_motor() {
  if (dc_motor_aan) {
    if(dc_motor_wijzerszin) {
      digitalWrite(Config::Conveyor::enablePin,HIGH);
      gewichtssensor();
    } else {
      digitalWrite(Config::Conveyor::reversePin,HIGH);
      digitalWrite(Config::Conveyor::enablePin,HIGH);
      dc_motor_aan = false;
      dc_motor_wijzerszin = true;
      delay(Config::Conveyor::reverseDelayMs);
      servo_wagen();
      check_bakken();
    }
  } else {
    digitalWrite(Config::Conveyor::enablePin,LOW);
  }
}


// Meet het gewicht en checkt wanneer de DC_motor moet omkere
void gewichtssensor() {
  collection.currentWeight = scale.get_units(); //scale.get_units() returns a float

  if (collection.selectedColor == beanbot::BeanColor::Black) {
    if (collection.currentWeight - collection.previousWeight >= order.blackGrams ) {
      dc_motor_wijzerszin = false;
      collection.previousWeight = collection.currentWeight;
      collection.selectedColor = beanbot::BeanColor::None;
    }
  }
  if (collection.selectedColor == beanbot::BeanColor::Red) {
    if (collection.currentWeight - collection.previousWeight >= order.redGrams ) {
      dc_motor_wijzerszin = false;
      collection.previousWeight = collection.currentWeight;
      collection.selectedColor = beanbot::BeanColor::None;
    }
  }
  if (collection.selectedColor == beanbot::BeanColor::White) {
    if (collection.currentWeight - collection.previousWeight >= order.whiteGrams ) {
      dc_motor_wijzerszin = false;
      collection.previousWeight = collection.currentWeight;
      collection.selectedColor = beanbot::BeanColor::None;
    }
  }
  String boodschap = String(collection.currentWeight);
  fromMonitorToApp(boodschap);
}


void check_bakken() {
  collection.reservoirsVisited += 1;
  if (collection.reservoirsVisited >= Config::reservoirCount) {
    start_bot = false;
    collection.reservoirsVisited = 0;
    stappen_motor_grote_stap_links();
    code_nog_niet_doorlopen = false;
    servo_bak();
    delay(Config::unloadDelayMs);
    servo_bak();
    withWifi = true;
  } else {
    stappen_motor_grote_stap_rechts();
    kleursensor_aan = true;
  }
}


// ---------------------------------- De loop ----------------------------------

void loop() {
  if (withWifi) {
    wifi();
  } else {
    start_zonder_wifi();
  }

  if (start_bot || start_metingen){
    kleursensor();
    dc_motor();
  }

  lcd_scherm();

  // Blijft data sturen naar MIT app zodat de app aan blijft
  if (code_nog_niet_doorlopen) {
    String boodschap = " ";
    fromMonitorToApp(boodschap);
  }
}
