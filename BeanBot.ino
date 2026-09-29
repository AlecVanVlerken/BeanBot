#include <Arduino.h>
#include "src/beanbot/Config.h"
#include "src/beanbot/Types.h"
#include "src/beanbot/RotatingFrame.h"
#include "src/beanbot/Conveyor.h"
#include "src/beanbot/ServoMechanisms.h"
#include "src/vendor/HX711/HX711.h"
#include "src/vendor/LiquidCrystal/LiquidCrystal.h"

// Existing communication and workflow remain procedural until their own stages.
bool withWifi = Config::Communication::useWifi;
String startsequence = Config::Communication::commandStart;
String stopsequence = Config::Communication::commandEnd;
String startsequenceSetup = Config::Communication::setupStart;
String stopsequenceSetup = Config::Communication::setupEnd;

beanbot::RotatingFrame frame;
beanbot::Conveyor conveyor;
beanbot::ServoMechanisms servos;
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

  conveyor.begin();
  servos.begin();
  frame.begin();

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

  frame.largeLeft();
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

    frame.largeLeft();
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
  bool surfaceDetected = false;

  while (servos.canStepProbe()) {
    afstandssensor();
    servos.stepProbe();
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
      surfaceDetected = true;
      break;
    }
  }

  if (surfaceDetected) {
    servos.returnProbe();
  } else {
    // Preserve the original extra downward command when the sweep expires.
    servos.stepProbe();
  }
  scan.reservoirsMeasured += 1;

  if (scan.reservoirsMeasured >= Config::reservoirCount) {
    start_metingen = false;
    scan.reservoirsMeasured = 0;
    frame.smallLeft();
    frame.largeLeft();
    frame.largeLeft();
    start_bot = true;
  } else {
    frame.smallRight();
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
        frame.smallRight();
        scan.firstDistance = true;
        afstandssensor_berekeningen();
      } else {
        if (order.redGrams > 0) {
          servos.extendCarriage();
          conveyor.feed();
        } else {
          check_bakken();
        }
      }
    } else if (Config::ColorSensor::greenMinBlack < colorReading.green) {
      kleursensor_aan = false;
      collection.selectedColor = beanbot::BeanColor::Black;
      if (start_metingen) {
        frame.smallRight();
        scan.firstDistance = true;
        afstandssensor_berekeningen();
      } else {
        if (order.redGrams > 0) {
          servos.extendCarriage();
          conveyor.feed();
        } else {
          check_bakken();
        }
      }
    } else {
      kleursensor_aan = false;
      collection.selectedColor = beanbot::BeanColor::Red;
      if (start_metingen) {
        frame.smallRight();
        scan.firstDistance = true;
        afstandssensor_berekeningen();
      } else {
        if (order.redGrams > 0) {
          servos.extendCarriage();
          conveyor.feed();
        } else {
          check_bakken();
        }
      }
    }
  }
}


// ---------------------------------- Functies voor DC-motor, gewichtssensor en checken van bakken ----------------------------------

// The workflow decides when to return excess; the mechanism only drives outputs.
void return_excess() {
  conveyor.reverse();
  delay(Config::Conveyor::reverseDelayMs);
  conveyor.stop();
  servos.retractCarriage();
  check_bakken();
}


// Meet het gewicht en checkt wanneer de DC_motor moet omkere
void gewichtssensor() {
  bool requestedWeightReached = false;
  collection.currentWeight = scale.get_units(); //scale.get_units() returns a float

  if (collection.selectedColor == beanbot::BeanColor::Black) {
    if (collection.currentWeight - collection.previousWeight >= order.blackGrams ) {
      requestedWeightReached = true;
      collection.previousWeight = collection.currentWeight;
      collection.selectedColor = beanbot::BeanColor::None;
    }
  }
  if (collection.selectedColor == beanbot::BeanColor::Red) {
    if (collection.currentWeight - collection.previousWeight >= order.redGrams ) {
      requestedWeightReached = true;
      collection.previousWeight = collection.currentWeight;
      collection.selectedColor = beanbot::BeanColor::None;
    }
  }
  if (collection.selectedColor == beanbot::BeanColor::White) {
    if (collection.currentWeight - collection.previousWeight >= order.whiteGrams ) {
      requestedWeightReached = true;
      collection.previousWeight = collection.currentWeight;
      collection.selectedColor = beanbot::BeanColor::None;
    }
  }
  String boodschap = String(collection.currentWeight);
  fromMonitorToApp(boodschap);
  if (requestedWeightReached) {
    return_excess();
  }
}


void check_bakken() {
  conveyor.stop();
  collection.reservoirsVisited += 1;
  if (collection.reservoirsVisited >= Config::reservoirCount) {
    start_bot = false;
    collection.reservoirsVisited = 0;
    frame.largeLeft();
    code_nog_niet_doorlopen = false;
    servos.openGate();
    delay(Config::unloadDelayMs);
    servos.closeGate();
    withWifi = true;
  } else {
    frame.largeRight();
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
    if (conveyor.isFeeding()) {
      gewichtssensor();
    }
  }

  lcd_scherm();

  // Blijft data sturen naar MIT app zodat de app aan blijft
  if (code_nog_niet_doorlopen) {
    String boodschap = " ";
    fromMonitorToApp(boodschap);
  }
}
