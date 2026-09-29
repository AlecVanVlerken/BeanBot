#ifndef BEANBOT_STOCK_DISPLAY_H
#define BEANBOT_STOCK_DISPLAY_H

#include "Types.h"
#include "../vendor/LiquidCrystal/LiquidCrystal.h"

namespace beanbot {

class StockDisplay {
public:
  StockDisplay();
  void begin();
  void show(const Stock& stock);

private:
  void printEntry(const StockEntry& entry, const char* label, int thirdLabelColumn);
  LiquidCrystal lcd_;
};

} // namespace beanbot

#endif
