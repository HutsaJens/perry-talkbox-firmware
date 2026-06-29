#include "sound_trigger.h"

namespace {
uint8_t g_triggerPin = 0;
bool g_triggerActive = false;
uint32_t g_triggerEndMs = 0;
portMUX_TYPE g_triggerMux = portMUX_INITIALIZER_UNLOCKED;
}

namespace sound_trigger {

void begin(uint8_t pin) {
  g_triggerPin = pin;
  pinMode(g_triggerPin, OUTPUT);
  digitalWrite(g_triggerPin, LOW);
}

void pulse(uint32_t durationMs) {
  portENTER_CRITICAL(&g_triggerMux);
  digitalWrite(g_triggerPin, HIGH);
  g_triggerActive = true;
  g_triggerEndMs = millis() + durationMs;
  portEXIT_CRITICAL(&g_triggerMux);
}

void tick() {
  bool shouldStopTrigger = false;

  portENTER_CRITICAL(&g_triggerMux);
  if (g_triggerActive && millis() >= g_triggerEndMs) {
    g_triggerActive = false;
    shouldStopTrigger = true;
  }
  portEXIT_CRITICAL(&g_triggerMux);

  if (shouldStopTrigger) {
    digitalWrite(g_triggerPin, LOW);
  }
}

}
