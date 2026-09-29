#ifndef BEANBOT_APPLICATION_H
#define BEANBOT_APPLICATION_H

#include "Config.h"
#include "Types.h"
#include "Communication.h"
#include "StockDisplay.h"
#include "RotatingFrame.h"
#include "Conveyor.h"
#include "ServoMechanisms.h"
#include "Scale.h"
#include "ColorSensor.h"
#include "DistanceSensor.h"

namespace beanbot {

// Owns order progress and the blocking sequence; components own device access.
class BeanBot {
public:
  void begin();
  void update();

private:
  enum class Phase { Waiting, Measuring, SelectingColor, Collecting };
  void startOrder(const Order& order);
  void measureCurrentReservoir();
  void selectCurrentColor();
  void collectCurrentColor();
  void returnExcess();
  void advanceReservoir();
  void finishOrder();
  int requestedGrams(BeanColor color) const;

  Communication communication_;
  StockDisplay display_;
  RotatingFrame frame_;
  Conveyor conveyor_;
  ServoMechanisms servos_;
  Scale scale_;
  ColorSensor colorSensor_;
  DistanceSensor distanceSensor_;

  Order order_ = Config::initialOrder;
  Stock stock_ = {
    {0, Config::Display::whitePosition},
    {0, Config::Display::blackPosition},
    {0, Config::Display::redPosition}
  };
  CollectionProgress collection_ = {0, 0, 0, BeanColor::None};
  ScanProgress scan_ = {0, 0, 0, true};
  Phase phase_ = Phase::Waiting;
  bool localDemoPending_ = false;
  bool orderCompleted_ = false;
};

} // namespace beanbot

#endif
