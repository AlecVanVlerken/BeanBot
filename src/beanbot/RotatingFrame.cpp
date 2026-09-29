#include "RotatingFrame.h"
#include "Config.h"
#include <Arduino.h>

namespace beanbot {

void RotatingFrame::begin() {
  pinMode(Config::Frame::stepPin, OUTPUT);
  pinMode(Config::Frame::directionPin, OUTPUT);
  pinMode(Config::Frame::enablePin, OUTPUT);
  digitalWrite(Config::Frame::enablePin, LOW);
}

void RotatingFrame::smallLeft() { step(false, Config::Frame::smallSteps); }
void RotatingFrame::largeLeft() { step(false, Config::Frame::largeSteps); }
void RotatingFrame::smallRight() { step(true, Config::Frame::smallSteps); }
void RotatingFrame::largeRight() { step(true, Config::Frame::largeSteps); }

void RotatingFrame::step(bool right, int count) {
  digitalWrite(Config::Frame::directionPin, right ? HIGH : LOW);
  for (int i = 0; i < count; ++i) {
    digitalWrite(Config::Frame::stepPin, HIGH);
    delay(Config::Frame::pulseDelayMs);
    digitalWrite(Config::Frame::stepPin, LOW);
    delay(Config::Frame::pulseDelayMs);
  }
  delay(Config::Frame::settleDelayMs);
}

} // namespace beanbot
