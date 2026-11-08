#pragma once

#include <stdint.h>

// Passive, allocation-free instrument for Wio Tracker L1 Companion's
// foreground loop. Samples exclude board.sleep(), which is intentional.
// All writes and reads occur in the same main-loop context (not DIO ISR).
struct LoopTimingSnapshot {
  uint32_t iterations = 0;
  uint32_t over10ms = 0;
  uint32_t over50ms = 0;
  uint32_t maxTotalUs = 0;
  uint32_t maxMeshUs = 0;
  uint32_t maxInterfaceUs = 0;
  uint32_t maxSensorsUs = 0;
  uint32_t maxUiUs = 0;
  uint32_t maxOtherUs = 0;
};

class LoopTimingMetrics {
public:
  static uint32_t elapsedUs(uint32_t start, uint32_t end) {
    // Unsigned wraparound is defined, including micros() rollover.
    return end - start;
  }

  void reset() { _snapshot = LoopTimingSnapshot{}; }

  void record(uint32_t meshUs, uint32_t interfaceUs, uint32_t sensorsUs,
              uint32_t uiUs, uint32_t otherUs, uint32_t totalUs) {
    increment(_snapshot.iterations);
    if (totalUs >= 10000u) increment(_snapshot.over10ms);
    if (totalUs >= 50000u) increment(_snapshot.over50ms);
    updateMax(_snapshot.maxTotalUs, totalUs);
    updateMax(_snapshot.maxMeshUs, meshUs);
    updateMax(_snapshot.maxInterfaceUs, interfaceUs);
    updateMax(_snapshot.maxSensorsUs, sensorsUs);
    updateMax(_snapshot.maxUiUs, uiUs);
    updateMax(_snapshot.maxOtherUs, otherUs);
  }

  LoopTimingSnapshot snapshot() const { return _snapshot; }

private:
  static void increment(uint32_t& v) {
    if (v != UINT32_MAX) ++v;
  }

  static void updateMax(uint32_t& v, uint32_t sample) {
    if (sample > v) v = sample;
  }

  LoopTimingSnapshot _snapshot {};
};
