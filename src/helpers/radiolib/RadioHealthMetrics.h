#pragma once

#include <stdint.h>

// Optional, low-overhead Companion radio diagnostics. Only the main-loop code
// updates these counters; the DIO ISR must never modify this structure.
struct RadioHealthSnapshot {
  uint32_t rx_arm_errors = 0;
  uint32_t rx_read_errors = 0;
  uint32_t tx_start_errors = 0;
  uint32_t agc_resets = 0;
};

class RadioHealthMetrics {
public:
  void reset() { _stats = RadioHealthSnapshot{}; }

  void recordRxArmError() { saturatingIncrement(_stats.rx_arm_errors); }
  void recordRxReadError() { saturatingIncrement(_stats.rx_read_errors); }
  void recordTxStartError() { saturatingIncrement(_stats.tx_start_errors); }
  void recordAgcReset() { saturatingIncrement(_stats.agc_resets); }

  RadioHealthSnapshot snapshot() const { return _stats; }

private:
  static void saturatingIncrement(uint32_t &value) {
    if (value != UINT32_MAX) ++value;
  }

  RadioHealthSnapshot _stats{};
};
