#include "wifi_manager.h"

#include <WiFi.h>

#include "logging.h"

namespace {
const char *g_ssid = nullptr;
const char *g_password = nullptr;
uint32_t g_connectTimeoutMs = 0;
uint32_t g_retryIntervalMs = 0;
uint32_t g_lastConnectAttemptMs = 0;

void tryConnect() {
  if (g_ssid == nullptr || g_password == nullptr) {
    return;
  }

  logging::log(String("\nConnecting to ") + g_ssid);
  WiFi.begin(g_ssid, g_password);

  uint32_t startMs = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - startMs) < g_connectTimeoutMs) {
    delay(500);
    logging::log(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    logging::logln("\nWiFi connected.");
  } else {
    logging::logln("\nWiFi connect timeout.");
  }

  g_lastConnectAttemptMs = millis();
}
}

namespace wifi_manager {

void begin(const char *ssid, const char *password, uint32_t connectTimeoutMs, uint32_t retryIntervalMs) {
  g_ssid = ssid;
  g_password = password;
  g_connectTimeoutMs = connectTimeoutMs;
  g_retryIntervalMs = retryIntervalMs;

  WiFi.mode(WIFI_STA);
  tryConnect();
}

void ensureConnected() {
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }

  if ((millis() - g_lastConnectAttemptMs) >= g_retryIntervalMs) {
    tryConnect();
  }
}

bool isConnected() {
  return WiFi.status() == WL_CONNECTED;
}

}
