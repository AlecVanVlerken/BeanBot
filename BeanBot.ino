#include <Arduino.h>
#include "src/beanbot/Config.h"
#include "src/beanbot/Types.h"
#include "src/beanbot/RotatingFrame.h"
#include "src/beanbot/Conveyor.h"
#include "src/beanbot/ServoMechanisms.h"
#include "src/beanbot/Scale.h"
#include "src/beanbot/ColorSensor.h"
#include "src/beanbot/DistanceSensor.h"
#include "src/beanbot/Inventory.h"
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
beanbot::Scale scale;
beanbot::ColorSensor colorSensor;
beanbot::DistanceSensor distanceSensor;
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
beanbot::ScanProgress scan = {0, 0, 0, true};

bool start_metingen = false;
bool start_bot = false;
bool kleursensor_aan = false;
bool code_nog_niet_doorlopen = true;

void setup() {
  // --> Wifi setup
  Serial2.begin(Config::Communication::baud); // Serial2 is de communicatie met de ESP32 (wifi-module).
  pinMode(Config::Communication::shieldPin, OUTPUT);
  digitalWrite(Config::Communication::shieldPin,HIGH);

  colorSensor.begin();

  conveyor.begin();
  servos.begin();
  frame.begin();

  scale.begin();

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

// Functie om de opslag bonen te bereken en wat er moet gebeuren na de berekening
void afstandssensor_berekeningen() {
  double elapsedSeconds = 0;
  bool surfaceDetected = false;

  while (servos.canStepProbe()) {
    scan.currentDistance = distanceSensor.readCentimeters();
    servos.stepProbe();
    // Keep the original arithmetic time model (2.5 * 10^-3 seconds).
    // Its mismatch with the physical probe delay is still unresolved.
    elapsedSeconds += Config::Inventory::timeIncrement * 1e-3;

    if (scan.firstDistance) {
      if (scan.currentDistance != 0) {
        scan.referenceDistance = scan.currentDistance;
        scan.firstDistance = false;
      }
      // A baseline sample must not also trigger surface detection.
      continue;
    }

    // Preserve <=; the intended surface margin/comparison remains ambiguous.
    if (scan.currentDistance <= scan.referenceDistance) {
      const double height = beanbot::Inventory::heightFromElapsedSeconds(elapsedSeconds);
      const double volume = beanbot::Inventory::volumeFromHeight(height);
      // Keep whole-gram stock storage; retain fractions throughout the calculation.
      const int weightGrams = static_cast<int>(
          beanbot::Inventory::weightGramsFromVolume(volume, collection.selectedColor));
      switch (collection.selectedColor) {
        case beanbot::BeanColor::Red:
          stock.red.weightGrams = weightGrams;
          break;
        case beanbot::BeanColor::White:
          stock.white.weightGrams = weightGrams;
          break;
        case beanbot::BeanColor::Black:
          stock.black.weightGrams = weightGrams;
          break;
        case beanbot::BeanColor::None:
          break;
      }
      collection.selectedColor = beanbot::BeanColor::None;
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


// Read the color, then make the existing workflow decisions here.
void process_reservoir() {
  if (!kleursensor_aan) {
    return;
  }
  collection.selectedColor = colorSensor.readColor();
  kleursensor_aan = false;
  if (start_metingen) {
    frame.smallRight();
    scan.firstDistance = true;
    afstandssensor_berekeningen();
  } else {
    // Preserve the existing request condition; per-color selection is stage 5.
    if (order.redGrams > 0) {
      servos.extendCarriage();
      conveyor.feed();
    } else {
      check_bakken();
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
void collect_requested_weight() {
  bool requestedWeightReached = false;
  collection.currentWeight = scale.readGrams(); // Preserve truncation to whole grams.

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
    process_reservoir();
    if (conveyor.isFeeding()) {
      collect_requested_weight();
    }
  }

  lcd_scherm();

  // Blijft data sturen naar MIT app zodat de app aan blijft
  if (code_nog_niet_doorlopen) {
    String boodschap = " ";
    fromMonitorToApp(boodschap);
  }
}
