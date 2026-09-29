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
#include "src/beanbot/Communication.h"
#include "src/beanbot/StockDisplay.h"

// The existing workflow still changes transport mode; stage 5 separates them.
beanbot::Communication communication;
beanbot::StockDisplay display;

beanbot::RotatingFrame frame;
beanbot::Conveyor conveyor;
beanbot::ServoMechanisms servos;
beanbot::Scale scale;
beanbot::ColorSensor colorSensor;
beanbot::DistanceSensor distanceSensor;
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
  communication.begin();

  colorSensor.begin();

  conveyor.begin();
  servos.begin();
  frame.begin();

  scale.begin();

  display.begin();
}


// ---------------------------------- Functies die gebruik maken van de wifi om het process in gang te steken ----------------------------------

void start_zonder_wifi() {
  communication.setUseWifi(false);
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
  if (communication.readOrder(order)) {
    communication.setUseWifi(false);
    code_nog_niet_doorlopen = true;
    collection.previousWeight = 0;
    collection.currentWeight = 0;
    collection.selectedColor = beanbot::BeanColor::None;
    scan.currentDistance = 0;

    frame.largeLeft();
    start_metingen = true;
    kleursensor_aan = true;
  }
  communication.forwardMonitorToApp();
}


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
  communication.sendWeight(collection.currentWeight);
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
    communication.setUseWifi(true);
  } else {
    frame.largeRight();
    kleursensor_aan = true;
  }
}


// ---------------------------------- De loop ----------------------------------

void loop() {
  if (communication.usesWifi()) {
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

  display.show(stock);

  // Blijft data sturen naar MIT app zodat de app aan blijft
  if (code_nog_niet_doorlopen) {
    communication.sendKeepalive();
  }
}
