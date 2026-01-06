# Autonomous Gas Sensor Node (ESP32) — ZE03-NH3 + SCD41

Battery-powered (deep-sleep) IoT node for vegetable storage monitoring:
- NH3: Winsen ZE03-NH3 (UART, 5V supply)
- CO2 + Temperature + Humidity: Sensirion SCD41 (I2C, 3.3V supply)
- Local buffering to LittleFS queue (JSON Lines) and bulk upload over MQTT when Wi‑Fi is available
- Deep sleep schedule (default: every 3 hours)

## Build / Flash (PlatformIO)
1. Install VSCode + PlatformIO.
2. Copy `include/secrets.example.h` to `include/secrets.h` and set Wi‑Fi/MQTT credentials.
3. (Optional) Adjust pins and warmup values in `include/config.h`.
4. Build: `pio run`
5. Upload: `pio run -t upload`
6. Upload filesystem (LittleFS): `pio run -t uploadfs`
7. Monitor: `pio device monitor`

## Notes
- NH3 and CO2 sensors have warmup/settling behavior. Tune warmup times in `config.h`.
- MQTT publishing uses 3 repeated publishes for telemetry (as requested). Queue ensures no data loss during outages.
- `include/secrets.h` is intentionally ignored by git.

## Wiring (minimal)
- ZE03-NH3: 5V + GND, UART 9600
  - ZE03 TX -> ESP32 RX (PIN_ZE03_RX)
  - ZE03 RX -> ESP32 TX (PIN_ZE03_TX)
- SCD41: 3.3V + GND, I2C SDA/SCL
