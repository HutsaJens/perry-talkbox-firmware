#pragma once

#include <Arduino.h>
#include <time.h>

struct SasAuthorization {
  String requestBaseUrl;
  String token;
};

namespace servicebus_sas {
bool createAuthorization(
  const String &connectionString,
  const String &topicName,
  const String &subscriptionName,
  time_t expiresAt,
  SasAuthorization &authorization,
  String &error
);

bool isClockFresh(
  time_t now,
  bool hasSuccessfulSync,
  uint32_t millisecondsSinceSync,
  uint32_t maximumSyncAgeMs
);
}
