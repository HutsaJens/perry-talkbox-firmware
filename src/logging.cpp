#include "logging.h"

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

namespace {
Print *g_serial = nullptr;
logging::SecondarySink g_sink = nullptr;
SemaphoreHandle_t g_logMutex = nullptr;
}

namespace logging {

void begin(uint32_t baudRate) {
  Serial.begin(baudRate);
  g_serial = &Serial;

  if (g_logMutex == nullptr) { 
    g_logMutex = xSemaphoreCreateMutex();
  }
}

void setSecondarySink(SecondarySink sink) {
  g_sink = sink;
}

void log(const String &message) {
  if (g_serial == nullptr || g_logMutex == nullptr) {
    return;
  }

  if (xSemaphoreTake(g_logMutex, portMAX_DELAY) == pdTRUE) {
    g_serial->print(message);

    if (g_sink != nullptr) {
      g_sink(message);
    }

    xSemaphoreGive(g_logMutex);
  }
}

void logln(const String &message) {
  if (g_serial == nullptr || g_logMutex == nullptr) {
    return;
  }

  if (xSemaphoreTake(g_logMutex, portMAX_DELAY) == pdTRUE) {
    g_serial->println(message);

    if (g_sink != nullptr) {
      g_sink(message + "\r\n");
    }

    xSemaphoreGive(g_logMutex);
  }
}

}
