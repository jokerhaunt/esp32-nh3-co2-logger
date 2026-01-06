#include "storage_queue.h"
#include "config.h"
#include <LittleFS.h>

namespace StorageQueue {

static bool ensureCapacity() {
  File f = LittleFS.open(QUEUE_PATH, "r");
  if (!f) return true;
  size_t sz = f.size();
  f.close();

  if (sz <= QUEUE_MAX_BYTES) return true;

  // Simple policy: if overflow, drop queue to prevent runaway.
  LittleFS.remove(QUEUE_PATH);
  return true;
}

bool begin() {
  if (!LittleFS.begin(true)) return false;
  ensureCapacity();
  return true;
}

size_t sizeBytes() {
  File f = LittleFS.open(QUEUE_PATH, "r");
  if (!f) return 0;
  size_t sz = f.size();
  f.close();
  return sz;
}

bool appendLine(const String& line) {
  ensureCapacity();
  File f = LittleFS.open(QUEUE_PATH, "a");
  if (!f) return false;
  bool ok = (f.print(line) > 0) && (f.print("\n") > 0);
  f.close();
  return ok;
}

bool hasData() {
  File f = LittleFS.open(QUEUE_PATH, "r");
  if (!f) return false;
  bool ok = f.size() > 0;
  f.close();
  return ok;
}

bool flushToMqtt(bool (*publishFn)(const char* payload)) {
  File in = LittleFS.open(QUEUE_PATH, "r");
  if (!in) return true; // nothing to send

  String line;
  while (in.available()) {
    line = in.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) continue;

    if (!publishFn(line.c_str())) {
      in.close();
      return false;
    }
    delay(20);
  }
  in.close();

  LittleFS.remove(QUEUE_PATH);
  return true;
}

} // namespace
