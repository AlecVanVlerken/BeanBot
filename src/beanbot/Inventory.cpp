#include "Inventory.h"
#include "Config.h"
#include <math.h>

namespace beanbot {
namespace Inventory {

double heightFromElapsedSeconds(double elapsedSeconds) {
  // Preserve the original lift model, including its factor of 60.
  return Config::Inventory::initialHeight -
      (60 * Config::Inventory::rpm * Config::Inventory::radius * elapsedSeconds) /
      (2 * Config::Inventory::pi);
}

double volumeFromHeight(double height) {
  // The report specifies degrees. Keep the sketch's geometry and coefficients;
  // the report's different geometry is not substituted here.
  const double angleRadians = Config::Inventory::angle * Config::Inventory::pi / 180.0;
  const double lowerVolume = Config::Inventory::base * height * height * 2 /
                             (3 * tan(angleRadians));
  if (height <= Config::Inventory::maximumHeight) {
    return lowerVolume;
  }
  const double excessHeight = height - Config::Inventory::maximumHeight;
  return lowerVolume + Config::Inventory::base * Config::Inventory::length * excessHeight +
      (6 + 2 * excessHeight * excessHeight * Config::Inventory::pi * excessHeight) / 6;
}

double weightGramsFromVolume(double volume, BeanColor color) {
  switch (color) {
    case BeanColor::White:
    case BeanColor::Red:
      return (volume / Config::Inventory::whiteRedBeanVolume) *
             Config::Inventory::whiteRedBeanWeight;
    case BeanColor::Black:
      return (volume / Config::Inventory::blackBeanVolume) *
             Config::Inventory::blackBeanWeight;
    case BeanColor::None:
      return 0;
  }
  return 0;
}

} // namespace Inventory
} // namespace beanbot
