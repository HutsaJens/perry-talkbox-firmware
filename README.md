# ESP32 Azure Service Bus Long Poll

PlatformIO project for ESP32 boards with:

- WiFi + Azure Service Bus long polling
- OTA updates using espota
- Telnet log mirror
- ISD1820 trigger pulse output

## Requirements

- VS Code with PlatformIO extension
- ESP32-C3 board (for example ESP32-C3 SuperMini)
- ESP32-WROOM board (for example ESP32 Dev Module)
- WiFi network access
- Azure Service Bus topic + subscription

## Build And Upload

### ESP32-C3

USB upload:

```powershell
pio run -e seeed_xiao_esp32c3 -t upload
```

OTA upload:

```powershell
pio run -e seeed_xiao_esp32c3_ota -t upload
```

If mDNS host resolution fails, upload OTA by IP:

```powershell
pio run -e seeed_xiao_esp32c3_ota -t upload --upload-port 192.168.x.x
```

### ESP32-WROOM

USB upload:

```powershell
pio run -e esp32_wroom_32 -t upload
```

OTA upload:

```powershell
pio run -e esp32_wroom_32_ota -t upload
```

If mDNS host resolution fails, upload OTA by IP:

```powershell
pio run -e esp32_wroom_32_ota -t upload --upload-port 192.168.x.x
```

### Notes

- OTA hostname is `perry-platypus.local`.
- Shared trigger pin default is GPIO21 for both board targets.
- For OTA, use upload only (do not combine upload and monitor in one command).

## Secrets Handling

This project reads local credentials from src/secrets.h and generates short-lived Service Bus SAS tokens on the ESP32.

- src/secrets.h is intentionally ignored by git.
- Never commit real WiFi credentials or Service Bus connection strings.
- Use a dedicated Service Bus policy with only `Listen` permission.
- Rotate the shared access key immediately if the connection string is ever exposed.

Example src/secrets.h template:

```cpp
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
```

The device synchronizes its UTC clock with SNTP, refreshes it hourly, and refuses to create SAS tokens if no successful synchronization has occurred for six hours.

## Formatting

- Prettier configuration is provided for project text/config files.
- C/C++ files should be formatted with clang-format.
