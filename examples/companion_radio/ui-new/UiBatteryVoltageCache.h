#pragma once

#include <stdint.h>

// Wio L1 OLED status-bar voltage sampling policy.
//
// The board's getBattMilliVolts() deliberately waits 10 ms for ADC setup.
// The status bar can reuse a recent sample on joystick-driven redraws
// without affecting the uncached battery shutdown/safety checks.
//
// This class does NOT perform ADC operations or trigger reads itself.
class UiBatteryVoltageCache {
public:
  static constexpr uint32_t RefreshMs = 5000u;

  bool needsSample(uint32_t nowMs) const {
    // Unsigned subtraction works across millis() wraparound.
    return !_valid || static_cast<uint32_t>(nowMs - _lastSampleMs) >= RefreshMs;
  }

  void store(uint32_t nowMs, uint16_t milliVolts) {
    _lastSampleMs = nowMs;
    _milliVolts = milliVolts;
    _valid = true;
  }

  uint16_t value() const { return _milliVolts; }
  bool hasValue() const { return _valid; }

  void invalidate() {
    _valid = false;
  }

private:
  uint32_t _lastSampleMs = 0;
  uint16_t _milliVolts = 0;
  bool _valid = false;
};
