#include "servicebus_sas.h"

#include <base64.h>
#include <mbedtls/md.h>

namespace {
static constexpr time_t MINIMUM_VALID_UNIX_TIME = 1704067200; // 2024-01-01 UTC

struct ConnectionInfo {
  String endpoint;
  String keyName;
  String key;
  String entityPath;
};

String percentEncode(const String &value) {
  static constexpr char HEX_DIGITS[] = "0123456789ABCDEF";
  String encoded;
  encoded.reserve(value.length() * 3);

  for (size_t index = 0; index < value.length(); ++index) {
    const uint8_t character = static_cast<uint8_t>(value[index]);
    if (
      (character >= 'A' && character <= 'Z') ||
      (character >= 'a' && character <= 'z') ||
      (character >= '0' && character <= '9') ||
      character == '-' || character == '_' || character == '.' || character == '~'
    ) {
      encoded += static_cast<char>(character);
    } else {
      encoded += '%';
      encoded += HEX_DIGITS[character >> 4];
      encoded += HEX_DIGITS[character & 0x0F];
    }
  }

  return encoded;
}

bool parseConnectionString(const String &connectionString, ConnectionInfo &info, String &error) {
  int start = 0;
  while (start <= static_cast<int>(connectionString.length())) {
    int end = connectionString.indexOf(';', start);
    if (end < 0) {
      end = connectionString.length();
    }

    String part = connectionString.substring(start, end);
    part.trim();
    if (!part.isEmpty()) {
      const int separator = part.indexOf('=');
      if (separator <= 0) {
        error = "Invalid Service Bus connection string field.";
        return false;
      }

      String name = part.substring(0, separator);
      String value = part.substring(separator + 1);
      name.trim();
      value.trim();

      if (name == "Endpoint") {
        info.endpoint = value;
      } else if (name == "SharedAccessKeyName") {
        info.keyName = value;
      } else if (name == "SharedAccessKey") {
        info.key = value;
      } else if (name == "EntityPath") {
        info.entityPath = value;
      }
    }

    if (end == static_cast<int>(connectionString.length())) {
      break;
    }
    start = end + 1;
  }

  if (info.endpoint.isEmpty() || info.keyName.isEmpty() || info.key.isEmpty()) {
    error = "Service Bus connection string is missing a required field.";
    return false;
  }

  return true;
}

bool endpointToBaseUrl(const String &endpoint, String &baseUrl, String &error) {
  String host;
  if (endpoint.startsWith("sb://")) {
    host = endpoint.substring(5);
  } else if (endpoint.startsWith("https://")) {
    host = endpoint.substring(8);
  } else {
    error = "Service Bus Endpoint must use sb:// or https://.";
    return false;
  }

  while (host.endsWith("/")) {
    host.remove(host.length() - 1);
  }

  if (host.isEmpty() || host.indexOf('/') >= 0 || host.indexOf('?') >= 0 || host.indexOf('#') >= 0) {
    error = "Service Bus Endpoint is malformed.";
    return false;
  }

  baseUrl = "https://" + host;
  return true;
}
}

namespace servicebus_sas {

bool createAuthorization(
  const String &connectionString,
  const String &topicName,
  const String &subscriptionName,
  time_t expiresAt,
  SasAuthorization &authorization,
  String &error
) {
  authorization = SasAuthorization{};
  error = "";

  if (topicName.isEmpty() || subscriptionName.isEmpty()) {
    error = "Service Bus topic and subscription are required.";
    return false;
  }

  ConnectionInfo info;
  if (!parseConnectionString(connectionString, info, error)) {
    return false;
  }

  if (!info.entityPath.isEmpty() && info.entityPath != topicName) {
    error = "Service Bus EntityPath does not match the configured topic.";
    return false;
  }

  String baseUrl;
  if (!endpointToBaseUrl(info.endpoint, baseUrl, error)) {
    return false;
  }

  authorization.requestBaseUrl =
    baseUrl + "/" + topicName + "/subscriptions/" + subscriptionName;
  const String encodedAudience = percentEncode(authorization.requestBaseUrl);
  const String expiry = String(static_cast<uint64_t>(expiresAt));
  const String stringToSign = encodedAudience + "\n" + expiry;

  uint8_t digest[32];
  const mbedtls_md_info_t *sha256 = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
  if (
    sha256 == nullptr ||
    mbedtls_md_hmac(
      sha256,
      reinterpret_cast<const unsigned char *>(info.key.c_str()),
      info.key.length(),
      reinterpret_cast<const unsigned char *>(stringToSign.c_str()),
      stringToSign.length(),
      digest
    ) != 0
  ) {
    error = "Failed to generate Service Bus SAS signature.";
    return false;
  }

  const String encodedSignature = percentEncode(base64::encode(digest, sizeof(digest)));
  authorization.token =
    "SharedAccessSignature sr=" + encodedAudience +
    "&sig=" + encodedSignature +
    "&se=" + expiry +
    "&skn=" + percentEncode(info.keyName);
  return true;
}

bool isClockFresh(
  time_t now,
  bool hasSuccessfulSync,
  uint32_t millisecondsSinceSync,
  uint32_t maximumSyncAgeMs
) {
  return hasSuccessfulSync && now >= MINIMUM_VALID_UNIX_TIME && millisecondsSinceSync <= maximumSyncAgeMs;
}

}
