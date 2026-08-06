#pragma once

#include <Arduino.h>

enum class PollResult {
  MessageReceived,
  NoContent,
  HttpError,
  ConnectionError,
  InitError
};

struct PollResponse {
  PollResult result;
  int httpCode;
  String payload;
  String error;
};

namespace servicebus_poller {
void beginTimeSync(
  const char *server1,
  const char *server2,
  uint32_t syncIntervalMs,
  uint32_t maximumSyncAgeMs
);

PollResponse receiveMessageLongPoll(
  const String &connectionString,
  const String &topicName,
  const String &subscriptionName,
  uint32_t sasTokenTtlSeconds,
  int pollingTimeoutSeconds,
  int httpTimeoutMs
);
}
