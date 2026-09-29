#ifndef BEANBOT_SCALE_H
#define BEANBOT_SCALE_H

#include "../vendor/HX711/HX711.h"

namespace beanbot {

class Scale {
public:
  void begin();
  float readGrams();

private:
  HX711 sensor_;
};

} // namespace beanbot

#endif
