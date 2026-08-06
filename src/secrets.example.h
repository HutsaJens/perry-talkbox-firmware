#pragma once
#include <Arduino.h>

const char *ssid = "YOUR_WIFI_SSID";
const char *password = "YOUR_WIFI_PASSWORD";

const String serviceBusConnectionString =
  "Endpoint=sb://YOUR_NAMESPACE.servicebus.windows.net/;"
  "SharedAccessKeyName=YOUR_LISTEN_POLICY;"
  "SharedAccessKey=YOUR_SHARED_ACCESS_KEY";

const String topicName = "YOUR_TOPIC";
const String subscriptionName = "YOUR_SUBSCRIPTION";
