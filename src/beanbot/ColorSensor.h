#ifndef BEANBOT_COLOR_SENSOR_H
#define BEANBOT_COLOR_SENSOR_H

#include "Types.h"

namespace beanbot {

class ColorSensor {
public:
  void begin();
  BeanColor readColor();

private:
  ColorReading readPulses();
  static BeanColor classify(const ColorReading& reading);
};

} // namespace beanbot

#endif
