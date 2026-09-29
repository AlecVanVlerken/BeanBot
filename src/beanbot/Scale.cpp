#include "Scale.h"
#include "Config.h"

namespace beanbot {

void Scale::begin() {
  sensor_.begin(Config::Scale::dataPin, Config::Scale::clockPin);
  sensor_.set_scale(Config::Scale::calibrationFactor);
  // Preserve the original startup tare and its empty-container assumption.
  sensor_.tare();
}

float Scale::readGrams() {
  return sensor_.get_units();
}

} // namespace beanbot
