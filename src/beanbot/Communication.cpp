#include "Communication.h"
#include <stdlib.h>
#include <string.h>

namespace beanbot {

void Communication::begin() {
  Serial.begin(Config::Communication::usbBaud);
  Serial2.begin(Config::Communication::baud);
  pinMode(Config::Communication::shieldPin, OUTPUT);
  digitalWrite(Config::Communication::shieldPin, HIGH);
}

bool Communication::extractFrame(const String& input, const char* start,
                                 const char* end, String& payload) {
  const int startIndex = input.indexOf(start);
  if (startIndex < 0) {
    return false;
  }
  const int payloadStart = startIndex + strlen(start);
  const int endIndex = input.indexOf(end, payloadStart);
  if (endIndex < 0) {
    return false;
  }
  payload = input.substring(payloadStart, endIndex);
  return true;
}

String Communication::readCommand() {
  String input;
  if (!useWifi_) {
    while (Serial.available()) {
      input = Serial.readString();
    }
    return input;
  }

  while (Serial2.available()) {
    input = Serial2.readString();
  }
  String payload;
  // Keep the original command-before-setup precedence and readString behavior.
  if (input.indexOf(Config::Communication::commandStart) >= 0) {
    if (extractFrame(input, Config::Communication::commandStart,
                     Config::Communication::commandEnd, payload)) {
      return payload;
    }
  } else if (extractFrame(input, Config::Communication::setupStart,
                          Config::Communication::setupEnd, payload)) {
    Serial.println(payload);
  }
  return String();
}

Order Communication::parseOrder(const String& command) {
  // Preserve white:black:red ordering and the existing atol conversion.
  const String white = command.substring(0, command.indexOf(':'));
  const String black = command.substring(command.indexOf(':') + 1, command.lastIndexOf(':'));
  const String red = command.substring(command.lastIndexOf(':') + 1, command.length());
  Order order;
  order.whiteGrams = atol(white.c_str());
  order.blackGrams = atol(black.c_str());
  order.redGrams = atol(red.c_str());
  return order;
}

bool Communication::readOrder(Order& order) {
  const String command = readCommand();
  if (command.length() == 0) {
    return false;
  }
  order = parseOrder(command);
  return true;
}

void Communication::sendMessage(const String& message) {
  if (useWifi_) {
    Serial2.println(message);
  } else {
    Serial.print("Message for Wifi: ");
    Serial.println(message);
  }
}

void Communication::forwardMonitorToApp() {
  if (useWifi_ && Serial.available()) {
    sendMessage(Serial.readString());
  }
}

void Communication::sendWeight(long grams) {
  sendMessage(String(grams));
}

void Communication::sendKeepalive() {
  sendMessage(" ");
}

} // namespace beanbot
