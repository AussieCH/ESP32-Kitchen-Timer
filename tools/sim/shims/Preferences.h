#pragma once
#include <map>
#include <string>
#include <string.h>
class Preferences {
  std::map<std::string, int> ints;
  std::map<std::string, std::string> blobs;
 public:
  bool begin(const char *, bool = false) { return true; }
  int  getInt(const char *k, int def = 0) { auto i = ints.find(k); return i == ints.end() ? def : i->second; }
  void putInt(const char *k, int v) { ints[k] = v; }
  bool getBool(const char *k, bool def = false) { auto i = ints.find(k); return i == ints.end() ? def : i->second != 0; }
  void putBool(const char *k, bool v) { ints[k] = v ? 1 : 0; }
  size_t getBytes(const char *k, void *dst, size_t n) {
    auto i = blobs.find(k); if (i == blobs.end()) return 0;
    size_t c = i->second.size() < n ? i->second.size() : n;
    memcpy(dst, i->second.data(), c); return c;
  }
  void putBytes(const char *k, const void *src, size_t n) { blobs[k] = std::string((const char *)src, n); }
};
