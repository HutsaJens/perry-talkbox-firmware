#include <Arduino.h>
#include <ArduinoOTA.h>
#include <WiFi.h>

#include "app_config.h"
#include "logging.h"
#include "secrets.h"
#include "servicebus_poller.h"
#include "sound_trigger.h"
#include "telnet_logger.h"
#include "wifi_manager.h"

TaskHandle_t g_azureTaskHandle = nullptr;

void azurePollTask(void *pvParameters);

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
      logging::logln("Opening long polling connection...");

      PollResponse response = servicebus_poller::receiveMessageLongPoll(
        serviceBusConnectionString,
        topicName,
        subscriptionName,
        APP_SAS_TOKEN_TTL_SECONDS,
        APP_POLLING_TIMEOUT_SECONDS,
        APP_HTTP_TIMEOUT_MS
      );

      if (response.result == PollResult::MessageReceived) {
        logging::logln("--- Event Received ---");
        logging::logln(response.payload);
        logging::logln("----------------------");

        sound_trigger::pulse(100);
        logging::logln("ISD1820 trigger pulse");
      } else if (response.result == PollResult::NoContent) {
        logging::logln("Long poll timeout reached. No events received.");
      } else {
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
