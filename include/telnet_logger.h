#pragma once

#include <Arduino.h>

namespace telnet_logger {
void begin(uint16_t port);
void handleClient();
void write(const String &message);
void recordEvent(bool passedFilter, const String &summary);
bool isClientConnected();
}
