#include "sensors.h"
#include "config.h"
#include <Wire.h>
#include "SparkFun_SCD4x_Arduino_Library.h"

static HardwareSerial ZE03Serial(2);

// SparkFun SCD4x library object
static SCD4x scd41(SCD4x_SENSOR_SCD41);

bool Sensors::begin() {
  // UART for ZE03
  ZE03Serial.begin(ZE03_BAUD, SERIAL_8N1, PIN_ZE03_RX, PIN_ZE03_TX);
  return true;
}

// -------------------- ZE03 (NH3) --------------------
uint8_t Sensors::ze03Checksum(const uint8_t* p, size_t n) const {
  // Winsen ZE03 checksum: (~sum(bytes[1..n-2])) + 1
  uint8_t sum = 0;
  for (size_t i = 1; i <= n - 2; i++) sum += p[i];
  return (uint8_t)((~sum) + 1);
}

bool Sensors::ze03SetQAMode() {
  // Send: FF 01 78 04 00 00 00 00 CS
  uint8_t cmd[9] = {0xFF, 0x01, 0x78, 0x04, 0, 0, 0, 0, 0};
  cmd[8] = ze03Checksum(cmd, 9);

  while (ZE03Serial.available()) ZE03Serial.read();
  ZE03Serial.write(cmd, 9);
  ZE03Serial.flush();

  // Receive: FF 78 01 00 00 00 00 00 CS (success)
  uint8_t resp[9] = {0};
  if (!ze03ReadFrame(resp, 500)) return false;

  if (resp[0] != 0xFF || resp[1] != 0x78) return false;
  if (resp[8] != ze03Checksum(resp, 9)) return false;
  return resp[2] == 0x01;
}

bool Sensors::ze03RequestConcentration() {
  // Send: FF 01 86 00 00 00 00 00 CS
  uint8_t cmd[9] = {0xFF, 0x01, 0x86, 0, 0, 0, 0, 0, 0};
  cmd[8] = ze03Checksum(cmd, 9);

  while (ZE03Serial.available()) ZE03Serial.read();
  ZE03Serial.write(cmd, 9);
  ZE03Serial.flush();
  return true;
}

bool Sensors::ze03ReadFrame(uint8_t out[9], uint32_t timeoutMs) {
  uint32_t start = millis();
  int idx = 0;
  while ((millis() - start) < timeoutMs) {
    if (ZE03Serial.available()) {
      out[idx++] = (uint8_t)ZE03Serial.read();
      if (idx >= 9) return true;
    }
  }
  return false;
}

bool Sensors::readNH3_ZE03(float& out_ppm) {
  // Put to Q&A mode and request concentration
  (void)ze03SetQAMode();
  ze03RequestConcentration();

  uint8_t frame[9] = {0};
  if (!ze03ReadFrame(frame, 1200)) return false;

  // Frame: FF 86 HB LB gasNo dec ... CS
  if (frame[0] != 0xFF || frame[1] != 0x86) return false;
  if (frame[8] != ze03Checksum(frame, 9)) return false;

  uint16_t raw = (uint16_t)frame[2] * 256u + (uint16_t)frame[3];
  uint8_t dec = frame[5]; // 0 => 1ppm; 1 => 0.1ppm; 2 => 0.01ppm

  float resolution = 1.0f;
  if (dec == 1) resolution = 0.1f;
  else if (dec == 2) resolution = 0.01f;

  out_ppm = raw * resolution;
  return isfinite(out_ppm);
}

// -------------------- SCD41 (CO2 + T + RH) --------------------
bool Sensors::scd41InitIfNeeded() {
  if (_scdInited) return true;

  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);

  // begin(measBegin, autoCalibrate, skipStopPeriodicMeasurements)
  // For deep-sleep single-shot operation, autoCalibrate is usually false.
  if (!scd41.begin(false, false, false)) {
    return false;
  }
  _scdInited = true;
  return true;
}

bool Sensors::readCO2_T_RH_SCD41(float& out_co2, float& out_t, float& out_rh) {
  if (!scd41InitIfNeeded()) return false;

  auto doSingleShot = [&](bool useResult) -> bool {
    if (!scd41.measureSingleShot()) return false;

    uint32_t start = millis();
    while (!scd41.readMeasurement()) {
      if (millis() - start > 9000) return false;
      delay(100);
    }

    if (useResult) {
      out_co2 = (float)scd41.getCO2();
      out_t   = scd41.getTemperature();
      out_rh  = scd41.getHumidity();
    }
    return true;
  };

  // First shot (settling)
  if (!doSingleShot(false)) return false;
  // Second shot (use)
  if (!doSingleShot(true))  return false;

  return isfinite(out_co2) && isfinite(out_t) && isfinite(out_rh);
}
