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
PollResponse receiveMessageLongPoll(
  const String &sbNamespace,
  const String &topicName,
  const String &subscriptionName,
  const String &sasToken,
  int pollingTimeoutSeconds,
  int httpTimeoutMs
);
}
