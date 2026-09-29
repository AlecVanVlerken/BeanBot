#include "StockDisplay.h"
#include "Config.h"

namespace beanbot {

StockDisplay::StockDisplay()
    : lcd_(Config::Display::rs, Config::Display::enablePin,
           Config::Display::d4, Config::Display::d5,
           Config::Display::d6, Config::Display::d7) {}

void StockDisplay::begin() {
  lcd_.begin(Config::Display::columns, Config::Display::rows);
}

void StockDisplay::printEntry(const StockEntry& entry, const char* label,
                              int thirdLabelColumn) {
  int labelColumn;
  int weightColumn;
  switch (entry.displayPosition) {
    case 1:
      labelColumn = weightColumn = 0;
      break;
    case 2:
      labelColumn = weightColumn = 6;
      break;
    case 3:
      labelColumn = thirdLabelColumn;
      weightColumn = 12;
      break;
    default:
      return;
  }
  lcd_.setCursor(labelColumn, 0);
  lcd_.print(label);
  lcd_.setCursor(weightColumn, 1);
  lcd_.print(entry.weightGrams);
  lcd_.print("g");
}

void StockDisplay::show(const Stock& stock) {
  lcd_.clear();
  // Preserve print order and the distinct third-position label columns.
  printEntry(stock.red, "ROOD ", 12);
  printEntry(stock.white, "WIT ", 13);
  printEntry(stock.black, "ZWART ", 11);
}

} // namespace beanbot
