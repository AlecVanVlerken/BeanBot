#include "ColorSensor.h"
#include "Config.h"
#include <Arduino.h>

namespace beanbot {

void ColorSensor::begin() {
  pinMode(Config::ColorSensor::s0, OUTPUT);
  pinMode(Config::ColorSensor::s1, OUTPUT);
  pinMode(Config::ColorSensor::s2, OUTPUT);
  pinMode(Config::ColorSensor::s3, OUTPUT);
  pinMode(Config::ColorSensor::outputPin, INPUT);
  pinMode(Config::ColorSensor::enablePin, OUTPUT);
  digitalWrite(Config::ColorSensor::enablePin, LOW);
  // Original 20% frequency scaling.
  digitalWrite(Config::ColorSensor::s0, HIGH);
  digitalWrite(Config::ColorSensor::s1, LOW);
}

ColorReading ColorSensor::readPulses() {
  ColorReading reading;
  digitalWrite(Config::ColorSensor::s2, LOW);
  digitalWrite(Config::ColorSensor::s3, LOW);
  reading.red = pulseIn(Config::ColorSensor::outputPin, LOW);
  delay(Config::ColorSensor::readingDelayMs);

  digitalWrite(Config::ColorSensor::s2, HIGH);
  digitalWrite(Config::ColorSensor::s3, HIGH);
  reading.green = pulseIn(Config::ColorSensor::outputPin, LOW);
  delay(Config::ColorSensor::readingDelayMs);

  digitalWrite(Config::ColorSensor::s2, LOW);
  digitalWrite(Config::ColorSensor::s3, HIGH);
  reading.blue = pulseIn(Config::ColorSensor::outputPin, LOW);
  delay(Config::ColorSensor::readingDelayMs);
  return reading;
}

BeanColor ColorSensor::classify(const ColorReading& reading) {
  if (reading.red < Config::ColorSensor::redMaxWhite) {
    return BeanColor::White;
  }
  if (reading.green > Config::ColorSensor::greenMinBlack) {
    return BeanColor::Black;
  }
  return BeanColor::Red;
}

BeanColor ColorSensor::readColor() {
  return classify(readPulses());
}

} // namespace beanbot
