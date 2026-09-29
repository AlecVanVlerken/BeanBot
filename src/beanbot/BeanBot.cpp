#include "BeanBot.h"
#include "Inventory.h"
#include <Arduino.h>

namespace beanbot {

void BeanBot::begin() {
  communication_.begin();
  colorSensor_.begin();
  conveyor_.begin();
  servos_.begin();
  frame_.begin();
  scale_.begin();
  display_.begin();
  localDemoPending_ = !communication_.usesWifi();
}

void BeanBot::update() {
  if (phase_ == Phase::Waiting) {
    communication_.forwardMonitorToApp();
    Order incoming;
    if (localDemoPending_) {
      // Run the existing demo once after boot, then await USB orders.
      // Automatic demo repetition was ambiguous in the original program.
      localDemoPending_ = false;
      startOrder(Config::demoOrder);
    } else if (communication_.readOrder(incoming)) {
      startOrder(incoming);
    }
  }

  switch (phase_) {
    case Phase::Waiting:
      break;
    case Phase::Measuring:
      measureCurrentReservoir();
      break;
    case Phase::SelectingColor:
      selectCurrentColor();
      if (phase_ == Phase::Collecting) {
        collectCurrentColor();
      }
      break;
    case Phase::Collecting:
      collectCurrentColor();
      break;
  }
  display_.show(stock_);
  if (!orderCompleted_) {
    communication_.sendKeepalive();
  }
}

void BeanBot::startOrder(const Order& order) {
  order_ = order;
  collection_ = {0, 0, 0, BeanColor::None};
  reservoirsMeasured_ = 0;
  orderCompleted_ = false;
  conveyor_.stop();
  frame_.largeLeft();
  phase_ = Phase::Measuring;
}

void BeanBot::measureCurrentReservoir() {
  collection_.selectedColor = colorSensor_.readColor();
  frame_.smallRight();
  bool firstDistance = true;
  long referenceDistance = 0;
  double elapsedSeconds = 0;

  while (servos_.canStepProbe()) {
    const long currentDistance = distanceSensor_.readCentimeters();
    const double sampledSeconds = elapsedSeconds;
    servos_.stepProbe();
    // Keep the original arithmetic time model (2.5 * 10^-3 seconds).
    // Its mismatch with the physical probe delay is still unresolved.
    elapsedSeconds += Config::Inventory::timeIncrement * 1e-3;

    // Zero is not a usable distance (it is also returned when no echo arrives).
    if (currentDistance == 0) {
      continue;
    }
    if (firstDistance) {
      referenceDistance = currentDistance;
      firstDistance = false;
      // A baseline sample must not also trigger surface detection.
      continue;
    }

    // The report describes beans interrupting the view of the rear wall.
    // An unchanged wall distance is not a detected bean surface.
    if (currentDistance < referenceDistance) {
      const double height = Inventory::heightFromElapsedSeconds(sampledSeconds);
      const double volume = Inventory::volumeFromHeight(height);
      // Keep whole-gram stock storage; retain fractions throughout the calculation.
      const int weightGrams = static_cast<int>(
          Inventory::weightGramsFromVolume(volume, collection_.selectedColor));
      switch (collection_.selectedColor) {
        case BeanColor::Red:
          stock_.red.weightGrams = weightGrams;
          break;
        case BeanColor::White:
          stock_.white.weightGrams = weightGrams;
          break;
        case BeanColor::Black:
          stock_.black.weightGrams = weightGrams;
          break;
        case BeanColor::None:
          break;
      }
      break;
    }
  }

  // Every exit must leave the probe ready for the next reservoir, including
  // an exhausted sweep. No detection leaves the existing stock estimate intact.
  servos_.returnProbe();
  collection_.selectedColor = BeanColor::None;
  reservoirsMeasured_ += 1;

  // Undo the probe offset before moving between color/collection positions.
  // Two small steps are not interchangeable with one calibrated large step.
  frame_.smallLeft();

  if (reservoirsMeasured_ >= Config::reservoirCount) {
    reservoirsMeasured_ = 0;
    frame_.largeLeft();
    frame_.largeLeft();
    phase_ = Phase::SelectingColor;
  } else {
    frame_.largeRight();
  }
}

int BeanBot::requestedGrams(BeanColor color) const {
  switch (color) {
    case BeanColor::White: return order_.whiteGrams;
    case BeanColor::Black: return order_.blackGrams;
    case BeanColor::Red: return order_.redGrams;
    case BeanColor::None: return 0;
  }
  return 0;
}

void BeanBot::selectCurrentColor() {
  collection_.selectedColor = colorSensor_.readColor();
  if (requestedGrams(collection_.selectedColor) > 0) {
    servos_.extendCarriage();
    conveyor_.feed();
    phase_ = Phase::Collecting;
  } else {
    advanceReservoir();
  }
}

void BeanBot::collectCurrentColor() {
  collection_.currentWeight = scale_.readGrams(); // Preserve whole-gram truncation.
  const bool reached = collection_.currentWeight - collection_.previousWeight >=
                       requestedGrams(collection_.selectedColor);
  if (reached) {
    updateRemainingStock(collection_.selectedColor,
                         collection_.currentWeight - collection_.previousWeight);
    // Keep the cumulative-weight baseline at the original comparison point.
    collection_.previousWeight = collection_.currentWeight;
    collection_.selectedColor = BeanColor::None;
  }
  communication_.sendWeight(collection_.currentWeight);
  if (reached) {
    returnExcess();
    advanceReservoir();
  }
}

void BeanBot::updateRemainingStock(BeanColor color, long collectedGrams) {
  StockEntry* entry = nullptr;
  switch (color) {
    case BeanColor::White: entry = &stock_.white; break;
    case BeanColor::Black: entry = &stock_.black; break;
    case BeanColor::Red: entry = &stock_.red; break;
    case BeanColor::None: return;
  }
  // The report asks for approximate stock before and after dispensing.
  // Deduct measured collection, not the requested mass; this is an estimate,
  // not a second distance measurement. A stock estimate cannot be negative.
  const long remaining = entry->weightGrams - collectedGrams;
  entry->weightGrams = remaining > 0 ? static_cast<int>(remaining) : 0;
}

void BeanBot::returnExcess() {
  conveyor_.reverse();
  delay(Config::Conveyor::reverseDelayMs);
  conveyor_.stop();
  servos_.retractCarriage();
}

void BeanBot::advanceReservoir() {
  conveyor_.stop();
  collection_.selectedColor = BeanColor::None;
  ++collection_.reservoirsVisited;
  if (collection_.reservoirsVisited >= Config::reservoirCount) {
    finishOrder();
  } else {
    frame_.largeRight();
    phase_ = Phase::SelectingColor;
  }
}

void BeanBot::finishOrder() {
  conveyor_.stop();
  frame_.largeLeft();
  servos_.openGate();
  delay(Config::unloadDelayMs);
  servos_.closeGate();
  // Report the collected mass, not a new reading of the now-empty container.
  communication_.sendWeight(collection_.currentWeight);
  orderCompleted_ = true;
  phase_ = Phase::Waiting;
}

} // namespace beanbot
