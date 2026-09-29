#include "ServoMechanisms.h"
#include <Arduino.h>

namespace beanbot {

void ServoMechanisms::begin() {
  driver_.begin();
  driver_.setOscillatorFrequency(Config::Servo::oscillatorHz);
  driver_.setPWMFreq(Config::Servo::frequencyHz);
}

void ServoMechanisms::writeAngle(int connector, int angle, int range) {
  const int pulse = map(angle, Config::Servo::minimumAngle, range,
                        Config::Servo::pulseMin, Config::Servo::pulseMax);
  driver_.setPWM(Config::Servo::lastChannel - connector, 0, pulse);
}

void ServoMechanisms::extendCarriage() {
  writeAngle(Config::Servo::carriageConnector, Config::Servo::carriageForward,
             Config::Servo::standardRange);
  delay(Config::Servo::travelDelayMs);
}

void ServoMechanisms::retractCarriage() {
  writeAngle(Config::Servo::carriageConnector, Config::Servo::carriageBack,
             Config::Servo::standardRange);
  delay(Config::Servo::travelDelayMs);
}

void ServoMechanisms::openGate() {
  writeAngle(Config::Servo::gateConnector, Config::Servo::gateOpen,
             Config::Servo::standardRange);
  delay(Config::Servo::travelDelayMs);
}

void ServoMechanisms::closeGate() {
  writeAngle(Config::Servo::gateConnector, Config::Servo::gateClosed,
             Config::Servo::standardRange);
  delay(Config::Servo::travelDelayMs);
}

bool ServoMechanisms::canStepProbe() const {
  return nextProbeStep_ <= Config::Servo::lastProbeStep;
}

void ServoMechanisms::stepProbe() {
  writeAngle(Config::Servo::probeConnector,
             Config::Servo::probeTop - nextProbeStep_, Config::Servo::probeRange);
  nextProbeStep_ += Config::Servo::probeStep;
  delayMicroseconds(Config::Servo::probeDelayUs);
}

void ServoMechanisms::returnProbe() {
  writeAngle(Config::Servo::probeConnector, Config::Servo::probeTop,
             Config::Servo::probeRange);
  // Returning is not a downward step: the next sweep starts at the first step.
  nextProbeStep_ = Config::Servo::firstProbeStep;
  delayMicroseconds(Config::Servo::probeDelayUs);
}

} // namespace beanbot
