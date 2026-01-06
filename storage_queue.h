#pragma once
#include <Arduino.h>

namespace StorageQueue {
  bool begin();
  bool appendLine(const String& line);
  bool hasData();
  bool flushToMqtt(bool (*publishFn)(const char* payload));
  size_t sizeBytes();
}
