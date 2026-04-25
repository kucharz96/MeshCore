#pragma once

#include <helpers/radiolib/CustomSX1262Wrapper.h>

#ifndef CARDPUTER_RF_STABILITY_RX_REASSERT_MS
#define CARDPUTER_RF_STABILITY_RX_REASSERT_MS 30000UL
#endif

class CardputerRfStabilityWrapper : public CustomSX1262Wrapper {
  uint32_t _last_rx_boost_check_ms = 0;

  void keepRxBoostedGain() {
#if defined(SX126X_RX_BOOSTED_GAIN) && SX126X_RX_BOOSTED_GAIN
    if (!getRxBoostedGainMode()) {
      setRxBoostedGainMode(true);
    }
#endif
  }

public:
  CardputerRfStabilityWrapper(CustomSX1262& radio, mesh::MainBoard& board)
    : CustomSX1262Wrapper(radio, board) { }

  bool startSendRaw(const uint8_t* bytes, int len) override {
    keepRxBoostedGain();
    return RadioLibWrapper::startSendRaw(bytes, len);
  }

  void onSendFinished() override {
    RadioLibWrapper::onSendFinished();
    keepRxBoostedGain();
  }

  int recvRaw(uint8_t* bytes, int sz) override {
    int len = RadioLibWrapper::recvRaw(bytes, sz);
    if (len > 0) {
      keepRxBoostedGain();
    }
    return len;
  }

  void loop() override {
    RadioLibWrapper::loop();
    uint32_t now = millis();
    if ((uint32_t)(now - _last_rx_boost_check_ms) >= CARDPUTER_RF_STABILITY_RX_REASSERT_MS) {
      _last_rx_boost_check_ms = now;
      keepRxBoostedGain();
    }
  }
};
