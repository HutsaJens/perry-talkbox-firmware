#pragma once

#include <Arduino.h>

namespace sound_trigger {
void begin(uint8_t pin);
void pulse(uint32_t durationMs);
void tick();
} 
 