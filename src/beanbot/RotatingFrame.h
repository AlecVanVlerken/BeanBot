#ifndef BEANBOT_ROTATING_FRAME_H
#define BEANBOT_ROTATING_FRAME_H

namespace beanbot {

class RotatingFrame {
public:
  void begin();
  void smallLeft();
  void largeLeft();
  void smallRight();
  void largeRight();

private:
  void step(bool right, int count);
};

} // namespace beanbot

#endif
