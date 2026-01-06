#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#include "config.h"
#include "secrets.h"

#include "power_switch.h"
#include "sensors.h"
#include "storage_queue.h"

#include <esp_sleep.h>

RTC_DATA_ATTR uint32_t bootCount = 0;

static WiFiClient wifiClient;
static PubSubClient mqtt(wifiClient);

static PowerSwitch pwrNH3(PIN_PWR_ZE03_NH3, PWR_ACTIVE_HIGH);
static PowerSwitch pwrSCD(PIN_PWR_SCD41,    PWR_ACTIVE_HIGH);

static Sensors sensors;

// NOTE: firstWarmupDone is kept in RTC memory; if power is fully removed, warmup repeats.
RTC_DATA_ATTR bool ze03FirstWarmupDone = false;

static float readVbat() {
  if (PIN_ADC_VBAT < 0) return NAN;
  analogReadResolution(12);
  uint16_t adc = analogRead(PIN_ADC_VBAT);
  float v = (adc / (float)ADC_MAX) * ADC_VREF * VBAT_DIVIDER_RATIO;
  return v;
}

static bool wifiConnect() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - start) < WIFI_CONNECT_TIMEOUT_MS) {
    delay(150);
  }
  return WiFi.status() == WL_CONNECTED;
}

static bool mqttConnect() {
  mqtt.setServer(MQTT_HOST, MQTT_PORT);

  String clientId = String(DEVICE_ID) + "-" + String((uint32_t)ESP.getEfuseMac(), HEX);
  if (strlen(MQTT_USER) > 0) {
    return mqtt.connect(clientId.c_str(), MQTT_USER, MQTT_PASS);
  }
  return mqtt.connect(clientId.c_str());
}

static bool publishTelemetryTriplicate(const char* payload) {
  // As requested: publish telemetry 3 times in a row.
  for (int i = 0; i < 3; i++) {
    if (!mqtt.publish(TOPIC_TELEMETRY, payload, false)) return false;
    mqtt.loop();
    delay(40);
  }
  return true;
}

static String makeJsonLine(const SensorReadings& r, float vbat) {
  StaticJsonDocument<320> doc;

  doc["device_id"] = DEVICE_ID;
  doc["fw"] = FW_VERSION;
  doc["boot"] = bootCount;
  doc["ts_ms"] = (uint32_t)(esp_timer_get_time() / 1000ULL);

  JsonObject s = doc.createNestedObject("sensors");
  if (isfinite(r.nh3_ppm)) s["nh3_ppm"] = r.nh3_ppm;
  if (isfinite(r.co2_ppm)) s["co2_ppm"] = r.co2_ppm;
  if (isfinite(r.temp_c))  s["t_c"]     = r.temp_c;
  if (isfinite(r.hum_pct)) s["rh_pct"]  = r.hum_pct;

  if (isfinite(vbat)) doc["vbat"] = vbat;

  String out;
  serializeJson(doc, out);
  return out;
}

static void goSleep() {
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);

  esp_sleep_enable_timer_wakeup((uint64_t)MEASURE_INTERVAL_SEC * 1000000ULL);
  esp_deep_sleep_start();
}

void setup() {
  Serial.begin(115200);
  delay(50);

  bootCount++;

  pwrNH3.begin();
  pwrSCD.begin();

  if (!StorageQueue::begin()) {
    goSleep();
  }

  sensors.begin();

  // ---- Measure ----
  SensorReadings r;
  float vbat = readVbat();

  // ZE03-NH3 (UART, typically 5V supply)
  if (pwrNH3.isUsed()) {
    pwrNH3.on();

    if (!ze03FirstWarmupDone) {
      delay(ZE03_FIRST_WARMUP_MS);
      ze03FirstWarmupDone = true;
    } else {
      delay(ZE03_WARMUP_MS);
    }

    sensors.readNH3_ZE03(r.nh3_ppm);

    pwrNH3.off();
  } else {
    // if always powered
    sensors.readNH3_ZE03(r.nh3_ppm);
  }

  // SCD41 (I2C, 3.3V)
  if (pwrSCD.isUsed()) {
    pwrSCD.on();
    delay(SCD41_POWERUP_MS);
    sensors.readCO2_T_RH_SCD41(r.co2_ppm, r.temp_c, r.hum_pct);
    pwrSCD.off();
  } else {
    sensors.readCO2_T_RH_SCD41(r.co2_ppm, r.temp_c, r.hum_pct);
  }

  // ---- Store locally (queue) ----
  String line = makeJsonLine(r, vbat);
  StorageQueue::appendLine(line);

  // ---- Try upload ----
  if (wifiConnect() && mqttConnect()) {
    // Status message
    {
      StaticJsonDocument<220> st;
      st["device_id"] = DEVICE_ID;
      st["fw"] = FW_VERSION;
      st["boot"] = bootCount;
      st["queue_bytes"] = (uint32_t)StorageQueue::sizeBytes();
      st["ip"] = WiFi.localIP().toString();
      st["rssi"] = WiFi.RSSI();

      String s;
      serializeJson(st, s);
      mqtt.publish(TOPIC_STATUS, s.c_str(), false);
    }

    // Flush buffered telemetry
    StorageQueue::flushToMqtt([](const char* payload) {
      return publishTelemetryTriplicate(payload);
    });

    mqtt.disconnect();
  }

  goSleep();
}

void loop() {}
