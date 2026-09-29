#ifndef BEANBOT_CONVEYOR_H
#define BEANBOT_CONVEYOR_H

namespace beanbot {

class Conveyor {
public:
  void begin();
  void feed();
  void reverse();
  void stop();
};

} // namespace beanbot

#endif
