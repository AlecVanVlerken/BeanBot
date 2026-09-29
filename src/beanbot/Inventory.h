#ifndef BEANBOT_INVENTORY_H
#define BEANBOT_INVENTORY_H

#include "Types.h"

namespace beanbot {
namespace Inventory {

double heightFromElapsedSeconds(double elapsedSeconds);
double volumeFromHeight(double height);
double weightGramsFromVolume(double volume, BeanColor color);

} // namespace Inventory
} // namespace beanbot

#endif
