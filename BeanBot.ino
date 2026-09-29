#include "src/beanbot/BeanBot.h"

beanbot::BeanBot beanBot;

void setup() {
  beanBot.begin();
}

void loop() {
  beanBot.update();
}
