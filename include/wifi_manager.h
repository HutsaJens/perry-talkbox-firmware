#pragma once

#include <Arduino.h>

namespace wifi_manager {
void begin(const char *ssid, const char *password, uint32_t connectTimeoutMs, uint32_t retryIntervalMs);
void ensureConnected();
bool isConnected();
}
