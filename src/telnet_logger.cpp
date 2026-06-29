#include "telnet_logger.h"

#include <WiFi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

namespace {
WiFiServer *g_server = nullptr;
WiFiClient g_client;
SemaphoreHandle_t g_telnetMutex = nullptr;
}

namespace telnet_logger {

void begin(uint16_t port) {
  if (g_telnetMutex == nullptr) {
    g_telnetMutex = xSemaphoreCreateMutex();
  }

  if (g_server == nullptr) {
    g_server = new WiFiServer(port);
  }

  g_server->begin();
  g_server->setNoDelay(true);
}

void handleClient() {
  if (g_server == nullptr || g_telnetMutex == nullptr) {
    return;
  }

  if (xSemaphoreTake(g_telnetMutex, portMAX_DELAY) == pdTRUE) {
    if (g_server->hasClient()) {
      if (!g_client || !g_client.connected()) {
        if (g_client) {
          g_client.stop();
        }

        g_client = g_server->available();
        g_client.println("--- Connected to ESP32-C3 Remote Log ---");
      } else {
        g_server->available().stop();
      }
    }

    xSemaphoreGive(g_telnetMutex);
  }
}

void write(const String &message) {
  if (g_telnetMutex == nullptr) {
    return;
  }

  if (xSemaphoreTake(g_telnetMutex, portMAX_DELAY) == pdTRUE) {
    if (g_client && g_client.connected()) {
      g_client.print(message);
    }

    xSemaphoreGive(g_telnetMutex);
  }
}

bool isClientConnected() {
  bool isConnected = false;

  if (g_telnetMutex == nullptr) {
    return false;
  }

  if (xSemaphoreTake(g_telnetMutex, portMAX_DELAY) == pdTRUE) {
    isConnected = g_client && g_client.connected();
    xSemaphoreGive(g_telnetMutex);
  }

  return isConnected;
}

}
