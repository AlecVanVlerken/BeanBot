#ifndef BEANBOT_CONVEYOR_H
#define BEANBOT_CONVEYOR_H

namespace beanbot {

class Conveyor {
public:
  void begin();
  void feed();
  void reverse();
  void stop();
  bool isFeeding() const { return state_ == State::Feeding; }

private:
  enum class State { Stopped, Feeding, Reversing };
  State state_ = State::Stopped;
};

} // namespace beanbot

#endif
