#include "telnet_logger.h"

#include <cstdio>
#include <cstring>
#include <WiFi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

namespace {
constexpr size_t kTriggerHistoryCapacity = 50;
constexpr size_t kSummaryBufferLength = 128;
constexpr size_t kCommandBufferLength = 48;

struct TriggerRecord {
  bool passedFilter;
  char summary[kSummaryBufferLength];
};

WiFiServer *g_server = nullptr;
WiFiClient g_client;
SemaphoreHandle_t g_telnetMutex = nullptr;
TriggerRecord g_triggerHistory[kTriggerHistoryCapacity] = {};
size_t g_triggerHistoryCount = 0;
size_t g_nextTriggerRecord = 0;
char g_commandBuffer[kCommandBufferLength] = {};
size_t g_commandLength = 0;
bool g_commandOverflow = false;

enum class HistoryFilter {
  Passed,
  Filtered,
  All
};

void printTriggerHistory(HistoryFilter filter) {
  switch (filter) {
    case HistoryFilter::Passed:
      g_client.println("Recent successful triggers:");
      break;
    case HistoryFilter::Filtered:
      g_client.println("Recent filtered events:");
      break;
    case HistoryFilter::All:
      g_client.println("Recent events:");
      break;
  }

  const size_t firstRecord = (g_nextTriggerRecord + kTriggerHistoryCapacity - g_triggerHistoryCount) % kTriggerHistoryCapacity;
  bool foundRecord = false;
  for (size_t index = 0; index < g_triggerHistoryCount; ++index) {
    const TriggerRecord &record = g_triggerHistory[(firstRecord + index) % kTriggerHistoryCapacity];
    const bool matches = filter == HistoryFilter::All ||
      (filter == HistoryFilter::Passed && record.passedFilter) ||
      (filter == HistoryFilter::Filtered && !record.passedFilter);
    if (matches) {
      g_client.println(record.summary);
      foundRecord = true;
    }
  }

  if (!foundRecord) {
    g_client.println("No matching events in recent history.");
  }
}

void executeCommand() {
  g_commandBuffer[g_commandLength] = '\0';

  if (std::strcmp(g_commandBuffer, "/help") == 0) {
    g_client.println("Commands:");
    g_client.println("  /show-triggers             Successful triggers only");
    g_client.println("  /show-triggers all         All recent events");
    g_client.println("  /show-triggers filtered    Events that did not trigger");
  } else if (std::strcmp(g_commandBuffer, "/show-triggers") == 0) {
    printTriggerHistory(HistoryFilter::Passed);
  } else if (std::strcmp(g_commandBuffer, "/show-triggers all") == 0) {
    printTriggerHistory(HistoryFilter::All);
  } else if (std::strcmp(g_commandBuffer, "/show-triggers filtered") == 0) {
    printTriggerHistory(HistoryFilter::Filtered);
  } else {
    g_client.println("Unknown command. Use /help.");
  }

  g_commandLength = 0;
}

void handleCommands() {
  while (g_client.available() > 0) {
    const int incoming = g_client.read();
    if (incoming < 0 || incoming == '\r') {
      continue;
    }

    if (incoming == '\n') {
      if (g_commandOverflow) {
        g_client.println("Command too long.");
      } else if (g_commandLength > 0) {
        executeCommand();
      }
      g_commandLength = 0;
      g_commandOverflow = false;
      continue;
    }

    if (g_commandLength + 1 < kCommandBufferLength) {
      g_commandBuffer[g_commandLength++] = static_cast<char>(incoming);
    } else {
      g_commandOverflow = true;
    }
  }
}
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
        g_client.println("Use /help to list commands.");
        g_commandLength = 0;
        g_commandOverflow = false;
      } else {
        g_server->available().stop();
      }
    }

    if (g_client && g_client.connected()) {
      handleCommands();
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

void recordEvent(bool passedFilter, const String &summary) {
  if (g_telnetMutex == nullptr) {
    return;
  }

  if (xSemaphoreTake(g_telnetMutex, portMAX_DELAY) == pdTRUE) {
    TriggerRecord &record = g_triggerHistory[g_nextTriggerRecord];
    record.passedFilter = passedFilter;
    std::snprintf(record.summary, sizeof(record.summary), "%s", summary.c_str());
    g_nextTriggerRecord = (g_nextTriggerRecord + 1) % kTriggerHistoryCapacity;
    if (g_triggerHistoryCount < kTriggerHistoryCapacity) {
      ++g_triggerHistoryCount;
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
