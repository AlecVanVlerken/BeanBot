#include "DistanceSensor.h"
#include "Config.h"
#include <Arduino.h>

namespace {
long microsecondsToCentimeters(long microseconds) {
  return microseconds / Config::DistanceSensor::microsecondsPerCentimeter / 2;
}
}

namespace beanbot {

long DistanceSensor::readCentimeters() {
  // Retain the original per-reading pin setup, trigger timing and pulseIn timeout.
  pinMode(Config::DistanceSensor::triggerPin, OUTPUT);
  digitalWrite(Config::DistanceSensor::triggerPin, LOW);
  delayMicroseconds(Config::DistanceSensor::triggerLowUs);
  digitalWrite(Config::DistanceSensor::triggerPin, HIGH);
  delayMicroseconds(Config::DistanceSensor::triggerHighUs);
  digitalWrite(Config::DistanceSensor::triggerPin, LOW);
  pinMode(Config::DistanceSensor::echoPin, INPUT);
  const long duration = pulseIn(Config::DistanceSensor::echoPin, HIGH);
  return microsecondsToCentimeters(duration);
}

} // namespace beanbot
