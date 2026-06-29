#pragma once

#include <Arduino.h>

#ifndef APP_SOUND_TRIGGER_PIN
#define APP_SOUND_TRIGGER_PIN 21
#endif

static constexpr uint32_t APP_SERIAL_BAUD = 115200;
static constexpr uint16_t APP_TELNET_PORT = 23;
static constexpr const char *APP_OTA_HOSTNAME = "perry-platypus";

static constexpr uint32_t APP_WIFI_CONNECT_TIMEOUT_MS = 20000;
static constexpr uint32_t APP_WIFI_RETRY_INTERVAL_MS = 5000;

static constexpr int APP_POLLING_TIMEOUT_SECONDS = 60;
static constexpr int APP_HTTP_TIMEOUT_MS = (APP_POLLING_TIMEOUT_SECONDS + 5) * 1000;

static constexpr uint32_t APP_MAIN_LOOP_DELAY_MS = 10;
static constexpr uint32_t APP_POLL_TASK_IDLE_DELAY_MS = 100;
static constexpr uint32_t APP_HTTP_ERROR_RETRY_DELAY_MS = 2000;

static constexpr uint32_t APP_AZURE_TASK_STACK_WORDS = 8192;
