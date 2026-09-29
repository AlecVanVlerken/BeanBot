#ifndef BEANBOT_SERVO_MECHANISMS_H
#define BEANBOT_SERVO_MECHANISMS_H

#include "Config.h"
#include "../vendor/Adafruit_PWMServoDriver/Adafruit_PWMServoDriver.h"

namespace beanbot {

class ServoMechanisms {
public:
  void begin();
  void extendCarriage();
  void retractCarriage();
  void openGate();
  void closeGate();
  bool canStepProbe() const;
  void stepProbe();
  void returnProbe();

private:
  void writeAngle(int connector, int angle, int range);
  Adafruit_PWMServoDriver driver_;
  // Explicit carriage/gate commands need no alternating-direction flags.
  // Only the probe needs persistent progress between commands.
  int nextProbeStep_ = Config::Servo::firstProbeStep;
};

} // namespace beanbot

#endif
