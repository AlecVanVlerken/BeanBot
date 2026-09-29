#include "Conveyor.h"
#include "Config.h"
#include <Arduino.h>

namespace beanbot {

void Conveyor::begin() {
  pinMode(Config::Conveyor::enablePin, OUTPUT);
  pinMode(Config::Conveyor::reversePin, OUTPUT);
  stop();
  digitalWrite(Config::Conveyor::reversePin, LOW);
}

void Conveyor::feed() {
  digitalWrite(Config::Conveyor::reversePin, LOW);
  digitalWrite(Config::Conveyor::enablePin, HIGH);
}

void Conveyor::reverse() {
  digitalWrite(Config::Conveyor::reversePin, HIGH);
  digitalWrite(Config::Conveyor::enablePin, HIGH);
}

void Conveyor::stop() {
  digitalWrite(Config::Conveyor::enablePin, LOW);
}

} // namespace beanbot
