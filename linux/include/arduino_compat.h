#pragma once

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string>
#include <sys/time.h>

inline uint32_t millis() {
  static const auto start = std::chrono::steady_clock::now();
  const auto now = std::chrono::steady_clock::now();
  return static_cast<uint32_t>(
      std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count());
}

template <typename T>
inline T constrain(T val, T lo, T hi) {
  return std::max(lo, std::min(val, hi));
}

class Stream {
 public:
  void print(const char *s) { fputs(s, stdout); }
  void print(const std::string &s) { fputs(s.c_str(), stdout); }
  void println(const char *s) {
    print(s);
    print("\n");
  }
};

inline Stream Serial;
