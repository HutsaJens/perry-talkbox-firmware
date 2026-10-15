#pragma once

#include <Arduino.h>

enum class MessageDisposition {
  Unresolved,
  Resolved,
  Invalid
};

struct ServiceBusEventDetails {
  String type = "<missing>";
  String eventName = "<missing>";
  String bodyName = "<missing>";
  String objectType = "<missing>";
  String createdAt = "<missing>";
  String createdBy = "<missing>";
  String sourceId = "<missing>";
  String ticketId = "<missing>";
  String alarmId = "<missing>";
};

struct MessageClassification {
  MessageDisposition disposition = MessageDisposition::Invalid;
  bool jsonValid = false;
  bool resolved = false;
  size_t payloadBytes = 0;
  ServiceBusEventDetails details;
  String error;
};

namespace servicebus_message {
MessageClassification classify(const String &payload);
}
