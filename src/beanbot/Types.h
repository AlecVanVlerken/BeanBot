#ifndef BEANBOT_TYPES_H
#define BEANBOT_TYPES_H

namespace beanbot {

enum class BeanColor { None, White, Black, Red };

struct Order {
  int whiteGrams;
  int blackGrams;
  int redGrams;
};

struct StockEntry {
  int weightGrams;
  int displayPosition;
};

struct Stock {
  StockEntry white;
  StockEntry black;
  StockEntry red;
};

struct CollectionProgress {
  long currentWeight;
  long previousWeight;
  int reservoirsVisited;
  BeanColor selectedColor;
};

struct ScanProgress {
  long referenceDistance;
  long currentDistance;
  long volume;
  int reservoirsMeasured;
  bool firstDistance;
};

struct ColorReading {
  int red;
  int green;
  int blue;
};

} // namespace beanbot

#endif
