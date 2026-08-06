#include <Arduino.h>
#include <unity.h>

#include "servicebus_sas.h"

namespace {
const String CONNECTION_STRING =
  "Endpoint=sb://example.servicebus.windows.net/;"
  "SharedAccessKeyName=Listen Policy;"
  "SharedAccessKey=abc123==;"
  "EntityPath=alerts";

void testCreatesExpectedAuthorization() {
  SasAuthorization authorization;
  String error;

  TEST_ASSERT_TRUE(servicebus_sas::createAuthorization(
    CONNECTION_STRING,
    "alerts",
    "devices",
    1704067500,
    authorization,
    error
  ));
  TEST_ASSERT_EQUAL_STRING(
    "https://example.servicebus.windows.net/alerts/subscriptions/devices",
    authorization.requestBaseUrl.c_str()
  );
  TEST_ASSERT_EQUAL_STRING(
    "SharedAccessSignature sr=https%3A%2F%2Fexample.servicebus.windows.net%2Falerts%2Fsubscriptions%2Fdevices&sig=6tsImVnyThabx1x8x9SEyrlPJSAtUkNrgnNdX%2FRFa0c%3D&se=1704067500&skn=Listen%20Policy",
    authorization.token.c_str()
  );
}

void testRejectsInvalidConnectionStrings() {
  SasAuthorization authorization;
  String error;

  TEST_ASSERT_FALSE(servicebus_sas::createAuthorization(
    "Endpoint=ftp://example.test/;SharedAccessKeyName=listen;SharedAccessKey=key",
    "alerts",
    "devices",
    1704067500,
    authorization,
    error
  ));
  TEST_ASSERT_FALSE(servicebus_sas::createAuthorization(
    "Endpoint=sb://example.test/;SharedAccessKeyName=listen",
    "alerts",
    "devices",
    1704067500,
    authorization,
    error
  ));
  TEST_ASSERT_FALSE(servicebus_sas::createAuthorization(
    CONNECTION_STRING,
    "other-topic",
    "devices",
    1704067500,
    authorization,
    error
  ));
}

void testClockFreshness() {
  static constexpr time_t VALID_TIME = 1704067200;
  static constexpr uint32_t SIX_HOURS_MS = 6UL * 60UL * 60UL * 1000UL;

  TEST_ASSERT_FALSE(servicebus_sas::isClockFresh(VALID_TIME, false, 0, SIX_HOURS_MS));
  TEST_ASSERT_FALSE(servicebus_sas::isClockFresh(1000, true, 0, SIX_HOURS_MS));
  TEST_ASSERT_TRUE(servicebus_sas::isClockFresh(VALID_TIME, true, SIX_HOURS_MS, SIX_HOURS_MS));
  TEST_ASSERT_FALSE(servicebus_sas::isClockFresh(VALID_TIME, true, SIX_HOURS_MS + 1, SIX_HOURS_MS));
}
}

void setup() {
  delay(2000);
  UNITY_BEGIN();
  RUN_TEST(testCreatesExpectedAuthorization);
  RUN_TEST(testRejectsInvalidConnectionStrings);
  RUN_TEST(testClockFreshness);
  UNITY_END();
}

void loop() {}
