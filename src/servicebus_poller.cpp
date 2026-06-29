#include "servicebus_poller.h"

#include <HTTPClient.h>
#include <WiFiClientSecure.h>

namespace servicebus_poller {

PollResponse receiveMessageLongPoll(
  const String &sbNamespace,
  const String &topicName,
  const String &subscriptionName,
  const String &sasToken,
  int pollingTimeoutSeconds,
  int httpTimeoutMs
) {
  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;

  String url =
    "https://" + sbNamespace +
    ".servicebus.windows.net/" +
    topicName +
    "/subscriptions/" +
    subscriptionName +
    "/messages/head?timeout=" +
    String(pollingTimeoutSeconds);

  PollResponse response;
  response.result = PollResult::InitError;
  response.httpCode = 0;

  if (!http.begin(client, url)) {
    response.error = "Failed to initialize HTTP Client.";
    return response;
  }

  http.addHeader("Authorization", sasToken);
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
