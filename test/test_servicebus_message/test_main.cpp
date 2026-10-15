#include <Arduino.h>
#include <unity.h>

#include "servicebus_message.h"

namespace {
const String SAMPLE_EVENT =
  R"json({"Type":"Event","Body":{"Object":{"TicketId":"INC0325719","AlarmId":"2ea6d72d-3ce3-452f-86da-6c1da5b51210","Resolved":false},"BodyName":"AgentFinishedEventBody"},"EventName":"AgentFinishedEvent","ObjectType":"Hoppenbrouwers.SM.Domain.Agents.AgentFinishedEventDetails","CreatedAt":"2026-08-06T12:23:31.4207806Z","CreatedBy":"","SourceId":"90042206-3092-4536-9fcc-e5c812343e0b"})json";

void testClassifiesSampleAsUnresolvedAndExtractsDetails() {
  const MessageClassification result = servicebus_message::classify(SAMPLE_EVENT);

  TEST_ASSERT_TRUE(result.jsonValid);
  TEST_ASSERT_EQUAL_INT(
    static_cast<int>(MessageDisposition::Unresolved),
    static_cast<int>(result.disposition)
  );
  TEST_ASSERT_FALSE(result.resolved);
  TEST_ASSERT_EQUAL_STRING("Event", result.details.type.c_str());
  TEST_ASSERT_EQUAL_STRING("AgentFinishedEvent", result.details.eventName.c_str());
  TEST_ASSERT_EQUAL_STRING("AgentFinishedEventBody", result.details.bodyName.c_str());
  TEST_ASSERT_EQUAL_STRING("INC0325719", result.details.ticketId.c_str());
  TEST_ASSERT_EQUAL_STRING("2ea6d72d-3ce3-452f-86da-6c1da5b51210", result.details.alarmId.c_str());
  TEST_ASSERT_EQUAL_STRING("<empty>", result.details.createdBy.c_str());
}

void testClassifiesResolvedEventWithoutTriggering() {
  const MessageClassification result = servicebus_message::classify(
    R"json({"Body":{"Object":{"Resolved":true}}})json"
  );

  TEST_ASSERT_TRUE(result.jsonValid);
  TEST_ASSERT_EQUAL_INT(
    static_cast<int>(MessageDisposition::Resolved),
    static_cast<int>(result.disposition)
  );
  TEST_ASSERT_TRUE(result.resolved);
}

void testDependsOnlyOnResolvedBoolean() {
  const MessageClassification result = servicebus_message::classify(
    "{ \"EventName\" : \"AnotherEvent\", \"Body\" : { \"Object\" : { \"Resolved\" : false } } }"
  );

  TEST_ASSERT_EQUAL_INT(
    static_cast<int>(MessageDisposition::Unresolved),
    static_cast<int>(result.disposition)
  );
  TEST_ASSERT_EQUAL_STRING("AnotherEvent", result.details.eventName.c_str());
  TEST_ASSERT_EQUAL_STRING("<missing>", result.details.ticketId.c_str());
}

void testRejectsMissingAndNonBooleanResolved() {
  MessageClassification result = servicebus_message::classify(
    R"json({"Type":42,"Body":{"Object":{}}})json"
  );
  TEST_ASSERT_TRUE(result.jsonValid);
  TEST_ASSERT_EQUAL_INT(
    static_cast<int>(MessageDisposition::Invalid),
    static_cast<int>(result.disposition)
  );
  TEST_ASSERT_EQUAL_STRING("<non-string>", result.details.type.c_str());
  TEST_ASSERT_EQUAL_STRING("<missing>", result.details.createdAt.c_str());

  result = servicebus_message::classify(
    R"json({"Body":{"Object":{"Resolved":"false"}}})json"
  );
  TEST_ASSERT_TRUE(result.jsonValid);
  TEST_ASSERT_EQUAL_INT(
    static_cast<int>(MessageDisposition::Invalid),
    static_cast<int>(result.disposition)
  );
}

void testRejectsMalformedDeepAndNonObjectJson() {
  MessageClassification result = servicebus_message::classify("{not-json}");
  TEST_ASSERT_FALSE(result.jsonValid);
  TEST_ASSERT_NOT_EQUAL(0, result.error.length());

  result = servicebus_message::classify("[[[[[[[0]]]]]]]");
  TEST_ASSERT_FALSE(result.jsonValid);
  TEST_ASSERT_NOT_EQUAL(0, result.error.length());

  result = servicebus_message::classify("[]");
  TEST_ASSERT_TRUE(result.jsonValid);
  TEST_ASSERT_EQUAL_INT(
    static_cast<int>(MessageDisposition::Invalid),
    static_cast<int>(result.disposition)
  );
  TEST_ASSERT_EQUAL_STRING("<missing>", result.details.eventName.c_str());
}
}

void setup() {
  delay(2000);
  UNITY_BEGIN();
  RUN_TEST(testClassifiesSampleAsUnresolvedAndExtractsDetails);
  RUN_TEST(testClassifiesResolvedEventWithoutTriggering);
  RUN_TEST(testDependsOnlyOnResolvedBoolean);
  RUN_TEST(testRejectsMissingAndNonBooleanResolved);
  RUN_TEST(testRejectsMalformedDeepAndNonObjectJson);
  UNITY_END();
}

void loop() {}
