#pragma once
#include <Arduino.h>

struct SensorReadings {
  float nh3_ppm = NAN;      // ZE03-NH3
  float co2_ppm = NAN;      // SCD41
  float temp_c  = NAN;      // SCD41
  float hum_pct = NAN;      // SCD41
};

class Sensors {
public:
  bool begin();

  bool readNH3_ZE03(float& out_ppm);
  bool readCO2_T_RH_SCD41(float& out_co2, float& out_t, float& out_rh);

private:
  // ZE03 helpers
  uint8_t ze03Checksum(const uint8_t* p, size_t n) const;
  bool ze03SetQAMode();              // 0x78 -> 0x04
  bool ze03RequestConcentration();   // 0x86
  bool ze03ReadFrame(uint8_t out[9], uint32_t timeoutMs);

  bool scd41InitIfNeeded();

private:
  bool _scdInited = false;
};
