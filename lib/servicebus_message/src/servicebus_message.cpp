#include "servicebus_message.h"

#include <ArduinoJson.h>

namespace {
static constexpr uint8_t MAXIMUM_JSON_NESTING = 6;

String readString(JsonVariantConst value) {
  if (value.isNull()) {
    return "<missing>";
  }
  if (!value.is<const char *>()) {
    return "<non-string>";
  }

  const String text = value.as<const char *>();
  return text.isEmpty() ? "<empty>" : text;
}
}

namespace servicebus_message {

MessageClassification classify(const String &payload) {
  MessageClassification classification;
  classification.payloadBytes = payload.length();

  JsonDocument document;
  const DeserializationError parseError = deserializeJson(
    document,
    payload,
    DeserializationOption::NestingLimit(MAXIMUM_JSON_NESTING)
  );
  if (parseError) {
    classification.error = String("JSON parsing failed: ") + parseError.c_str();
    return classification;
  }

  classification.jsonValid = true;
  if (!document.is<JsonObject>()) {
    classification.error = "JSON root must be an object.";
    return classification;
  }

  const JsonObjectConst root = document.as<JsonObjectConst>();
  classification.details.type = readString(root["Type"]);
  classification.details.eventName = readString(root["EventName"]);
  classification.details.bodyName = readString(root["Body"]["BodyName"]);
  classification.details.objectType = readString(root["ObjectType"]);
  classification.details.createdAt = readString(root["CreatedAt"]);
  classification.details.createdBy = readString(root["CreatedBy"]);
  classification.details.sourceId = readString(root["SourceId"]);
  classification.details.ticketId = readString(root["Body"]["Object"]["TicketId"]);
  classification.details.alarmId = readString(root["Body"]["Object"]["AlarmId"]);

  const JsonVariantConst resolved = root["Body"]["Object"]["Resolved"];
  if (!resolved.is<bool>()) {
    classification.error = "Body.Object.Resolved is missing or is not a boolean.";
    return classification;
  }

  classification.resolved = resolved.as<bool>();
  classification.disposition = classification.resolved
    ? MessageDisposition::Resolved
    : MessageDisposition::Unresolved;
  return classification;
}

}
