#pragma once

#include <Arduino.h>

namespace logging {
using SecondarySink = void (*)(const String &message);

void begin(uint32_t baudRate);
void setSecondarySink(SecondarySink sink);
void log(const String &message);
void logln(const String &message);
}
