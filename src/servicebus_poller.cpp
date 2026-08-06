#include "servicebus_poller.h"

#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <esp_sntp.h>
#include <time.h>

#include "servicebus_sas.h"

namespace {
volatile bool g_hasSuccessfulTimeSync = false;
volatile uint32_t g_lastTimeSyncMs = 0;
uint32_t g_maximumSyncAgeMs = 0;

void onTimeSynchronized(timeval *timeValue) {
  (void)timeValue;
  g_lastTimeSyncMs = millis();
  g_hasSuccessfulTimeSync = true;
}
}

namespace servicebus_poller {

void beginTimeSync(
  const char *server1,
  const char *server2,
  uint32_t syncIntervalMs,
  uint32_t maximumSyncAgeMs
) {
  g_hasSuccessfulTimeSync = false;
  g_lastTimeSyncMs = 0;
  g_maximumSyncAgeMs = maximumSyncAgeMs;

  esp_sntp_set_time_sync_notification_cb(onTimeSynchronized);
  configTime(0, 0, server1, server2);
  esp_sntp_set_sync_interval(syncIntervalMs);
  esp_sntp_restart();
}

PollResponse receiveMessageLongPoll(
  const String &connectionString,
  const String &topicName,
  const String &subscriptionName,
  uint32_t sasTokenTtlSeconds,
  int pollingTimeoutSeconds,
  int httpTimeoutMs
) {
  PollResponse response;
  response.result = PollResult::InitError;
  response.httpCode = 0;

  const time_t now = time(nullptr);
  const bool hasSuccessfulSync = g_hasSuccessfulTimeSync;
  const uint32_t millisecondsSinceSync = millis() - g_lastTimeSyncMs;
  if (!servicebus_sas::isClockFresh(now, hasSuccessfulSync, millisecondsSinceSync, g_maximumSyncAgeMs)) {
    response.error = "Clock has not synchronized recently enough to generate a SAS token.";
    return response;
  }

  SasAuthorization authorization;
  if (!servicebus_sas::createAuthorization(
        connectionString,
        topicName,
        subscriptionName,
        now + sasTokenTtlSeconds,
        authorization,
        response.error
      )) {
    return response;
  }

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;

  String url =
    authorization.requestBaseUrl +
    "/messages/head?timeout=" +
    String(pollingTimeoutSeconds);

  if (!http.begin(client, url)) {
    response.error = "Failed to initialize HTTP Client.";
    return response;
  }

  http.addHeader("Authorization", authorization.token);
  http.setTimeout(httpTimeoutMs);

  int httpCode = http.sendRequest("DELETE");
  response.httpCode = httpCode;

  if (httpCode > 0) {
    if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_CREATED) {
      response.result = PollResult::MessageReceived;
      response.payload = http.getString();
    } else if (httpCode == HTTP_CODE_NO_CONTENT) {
      response.result = PollResult::NoContent;
    } else {
      response.result = PollResult::HttpError;
      response.error = "HTTP Error: " + String(httpCode);
    }
  } else {
    response.result = PollResult::ConnectionError;
    response.error = "Connection failed, error: " + http.errorToString(httpCode);
  }

  http.end();
  return response;
}

}
