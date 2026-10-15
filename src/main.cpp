#include <Arduino.h>
#include <ArduinoOTA.h>
#include <WiFi.h>

#include "app_config.h"
#include "logging.h"
#include "secrets.h"
#include "servicebus_message.h"
#include "servicebus_poller.h"
#include "sound_trigger.h"
#include "telnet_logger.h"
#include "wifi_manager.h"

TaskHandle_t g_azureTaskHandle = nullptr;

void azurePollTask(void *pvParameters);

namespace {
void logEventField(const char *name, const String &value) {
  logging::logln(String(name) + ": " + value);
}

void logEvent(const MessageClassification &classification, bool gpioTriggered) {
  logging::logln("--- Event Received ---");

  if (!classification.jsonValid) {
    logging::logln("Classification: INVALID JSON");
    logging::logln(String("Reason: ") + classification.error);
    logging::logln(String("Payload bytes: ") + classification.payloadBytes);
    logging::logln("GPIO: not triggered");
    logging::logln("----------------------");
    telnet_logger::recordEvent(false, String("INVALID JSON: ") + classification.error);
    return;
  }

  switch (classification.disposition) {
    case MessageDisposition::Unresolved:
      logging::logln("Classification: UNRESOLVED");
      break;
    case MessageDisposition::Resolved:
      logging::logln("Classification: RESOLVED");
      break;
    case MessageDisposition::Invalid:
      logging::logln("Classification: INVALID EVENT");
      break;
  }

  logEventField("Type", classification.details.type);
  logEventField("EventName", classification.details.eventName);
  logEventField("BodyName", classification.details.bodyName);
  logEventField("ObjectType", classification.details.objectType);
  logEventField("CreatedAt", classification.details.createdAt);
  logEventField("CreatedBy", classification.details.createdBy);
  logEventField("SourceId", classification.details.sourceId);
  logEventField("TicketId", classification.details.ticketId);
  logEventField("AlarmId", classification.details.alarmId);

  if (classification.disposition == MessageDisposition::Invalid) {
    logging::logln("Resolved: <invalid>");
    logging::logln(String("Reason: ") + classification.error);
  } else {
    logging::logln(String("Resolved: ") + (classification.resolved ? "true" : "false"));
  }

  logging::logln(gpioTriggered ? "GPIO: triggered for 100 ms" : "GPIO: not triggered");
  logging::logln("----------------------");

  String summary;
  switch (classification.disposition) {
    case MessageDisposition::Unresolved:
      summary = "UNRESOLVED";
      break;
    case MessageDisposition::Resolved:
      summary = "RESOLVED";
      break;
    case MessageDisposition::Invalid:
      summary = "INVALID EVENT";
      break;
  }
  if (classification.details.eventName.length() > 0) {
    summary += String(": ") + classification.details.eventName;
  }
  if (classification.disposition == MessageDisposition::Invalid) {
    summary += String(" | ") + classification.error;
  }
  summary += gpioTriggered ? " | GPIO triggered" : " | GPIO not triggered";
  telnet_logger::recordEvent(gpioTriggered, summary);
}
}

void setup() {
  logging::begin(APP_SERIAL_BAUD);

  sound_trigger::begin(APP_SOUND_TRIGGER_PIN);

  wifi_manager::begin(ssid, password, APP_WIFI_CONNECT_TIMEOUT_MS, APP_WIFI_RETRY_INTERVAL_MS);

  servicebus_poller::beginTimeSync(
    APP_SNTP_SERVER_1,
    APP_SNTP_SERVER_2,
    APP_SNTP_SYNC_INTERVAL_MS,
    APP_SNTP_MAX_AGE_MS
  );

  telnet_logger::begin(APP_TELNET_PORT);
  logging::setSecondarySink(telnet_logger::write);

  logging::logln(String("IP Address: ") + WiFi.localIP().toString());

  ArduinoOTA.setHostname(APP_OTA_HOSTNAME);
  ArduinoOTA.begin();

  xTaskCreate(
    azurePollTask,
    "AzureTask",
    APP_AZURE_TASK_STACK_WORDS,
    nullptr,
    1,
    &g_azureTaskHandle
  );
}

void loop() {
  ArduinoOTA.handle();
  telnet_logger::handleClient();
  sound_trigger::tick();

  delay(APP_MAIN_LOOP_DELAY_MS);
}

void azurePollTask(void *pvParameters) {
  (void)pvParameters;

  for (;;) {
    if (wifi_manager::isConnected()) {
      PollResponse response = servicebus_poller::receiveMessageLongPoll(
        serviceBusConnectionString,
        topicName,
        subscriptionName,
        APP_SAS_TOKEN_TTL_SECONDS,
        APP_POLLING_TIMEOUT_SECONDS,
        APP_HTTP_TIMEOUT_MS
      );

      if (response.result == PollResult::MessageReceived) {
        const MessageClassification classification = servicebus_message::classify(response.payload);
        const bool shouldTrigger = classification.disposition == MessageDisposition::Resolved;

        if (shouldTrigger) {
          sound_trigger::pulse(100);
        }
        logEvent(classification, shouldTrigger);
        } else if (response.result != PollResult::NoContent) {
        logging::logln(response.error);
        vTaskDelay(APP_HTTP_ERROR_RETRY_DELAY_MS / portTICK_PERIOD_MS);
      }
    } else {
      logging::logln("WiFi disconnected. Reconnecting...");
      wifi_manager::ensureConnected();
    }

    vTaskDelay(APP_POLL_TASK_IDLE_DELAY_MS / portTICK_PERIOD_MS);
  }
}
