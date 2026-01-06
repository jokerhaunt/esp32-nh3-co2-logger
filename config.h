#pragma once
#include <Arduino.h>

// ===== Device =====
static const char* DEVICE_ID = "gph-gasnode-001";
static const char* FW_VERSION = "2.0.0";

// ===== Schedule =====
static const uint32_t MEASURE_INTERVAL_SEC = 3UL * 60UL * 60UL; // 3 hours

// ===== MQTT Topics =====
static const char* TOPIC_TELEMETRY = "gph/gasnode/telemetry";
static const char* TOPIC_STATUS    = "gph/gasnode/status";

// ===== Queue (LittleFS) =====
static const char* QUEUE_PATH = "/queue.jsonl";
static const size_t QUEUE_MAX_BYTES = 256 * 1024;

// ===== Power gating pins (optional) =====
// If = -1 -> sensor is NOT power-cycled by GPIO (stays powered).
static const int PIN_PWR_ZE03_NH3 = 25; // 5V to ZE03 through a switch (optocoupler/MOSFET)
static const int PIN_PWR_SCD41    = -1; // usually better not to hard power-cycle CO2 sensor

static const bool PWR_ACTIVE_HIGH = true;

// ===== Warmups =====
// ZE03: first boot warmup; adjust to your real sensor requirements.
static const uint32_t ZE03_FIRST_WARMUP_MS = 5UL * 60UL * 1000UL; // 5 min
static const uint32_t ZE03_WARMUP_MS       = 10UL * 1000UL;       // subsequent wakeups
static const uint32_t SCD41_POWERUP_MS     = 50;                  // only if SCD41 is power-cycled

// ===== ZE03 UART =====
static const int PIN_ZE03_RX = 16;  // ESP32 RX <- ZE03 TX
static const int PIN_ZE03_TX = 17;  // ESP32 TX -> ZE03 RX
static const uint32_t ZE03_BAUD = 9600;

// ===== I2C for SCD41 =====
static const int PIN_I2C_SDA = 21;
static const int PIN_I2C_SCL = 22;

// ===== Battery ADC (optional) =====
static const int PIN_ADC_VBAT = 35;           // -1 if not used
static const float VBAT_DIVIDER_RATIO = 2.0f; // adjust to your divider
static const float ADC_VREF = 3.3f;
static const int ADC_MAX = 4095;

// ===== Wi-Fi policy =====
static const uint32_t WIFI_CONNECT_TIMEOUT_MS = 15000;
