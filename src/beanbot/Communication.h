#ifndef BEANBOT_COMMUNICATION_H
#define BEANBOT_COMMUNICATION_H

#include <Arduino.h>
#include "Config.h"
#include "Types.h"

namespace beanbot {

class Communication {
public:
  void begin();
  bool usesWifi() const { return useWifi_; }
  void setUseWifi(bool enabled) { useWifi_ = enabled; }
  bool readOrder(Order& order);
  void forwardMonitorToApp();
  // Also used for the final numeric result when completion is wired in stage 5.
  void sendWeight(long grams);
  void sendKeepalive();

private:
  String readCommand();
  static Order parseOrder(const String& command);
  static bool extractFrame(const String& input, const char* start,
                           const char* end, String& payload);
  void sendMessage(const String& message);
  bool useWifi_ = Config::Communication::useWifi;
};

} // namespace beanbot

#endif
