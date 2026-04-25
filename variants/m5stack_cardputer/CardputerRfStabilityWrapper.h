#pragma once

#include <helpers/radiolib/CustomSX1262Wrapper.h>

#ifndef CARDPUTER_RF_STABILITY_RX_REASSERT_MS
#define CARDPUTER_RF_STABILITY_RX_REASSERT_MS 30000UL
#endif

#ifndef CARDPUTER_RF_TX_QUIET_WINDOW_MS
#define CARDPUTER_RF_TX_QUIET_WINDOW_MS 650UL
#endif

#ifndef CARDPUTER_RF_TX_RSSI_MARGIN_DB
#define CARDPUTER_RF_TX_RSSI_MARGIN_DB 10
#endif

#ifndef CARDPUTER_RF_TX_DEFAULT_BUSY_RSSI
#define CARDPUTER_RF_TX_DEFAULT_BUSY_RSSI -92
#endif

#ifndef CARDPUTER_RF_TX_SAMPLE_MS
#define CARDPUTER_RF_TX_SAMPLE_MS 18UL
#endif

class CardputerRfStabilityWrapper : public CustomSX1262Wrapper {
  uint32_t _last_rx_boost_check_ms = 0;
  uint16_t _last_tx_wait_ms = 0;
  uint32_t _tx_window_count = 0;

  void keepRxBoostedGain() {
#if defined(SX126X_RX_BOOSTED_GAIN) && SX126X_RX_BOOSTED_GAIN
    if (!getRxBoostedGainMode()) {
      setRxBoostedGainMode(true);
    }
#endif
  }

  int txBusyRssiThreshold() const {
    if (_noise_floor < 0) {
      return _noise_floor + CARDPUTER_RF_TX_RSSI_MARGIN_DB;
    }
    return CARDPUTER_RF_TX_DEFAULT_BUSY_RSSI;
  }

  bool txWindowBusy() {
    if (isReceivingPacket()) {
      return true;
    }
    return ((int)getCurrentRSSI()) > txBusyRssiThreshold();
  }

  void waitForTxWindow() {
    const uint32_t started = millis();
    uint32_t quiet_since = 0;
    _last_tx_wait_ms = 0;

    while ((uint32_t)(millis() - started) < CARDPUTER_RF_TX_QUIET_WINDOW_MS) {
      if (!txWindowBusy()) {
        if (quiet_since == 0) {
          quiet_since = millis();
        }
        if ((uint32_t)(millis() - quiet_since) >= (CARDPUTER_RF_TX_SAMPLE_MS * 2UL)) {
          break;
        }
      } else {
        quiet_since = 0;
      }

      delay(CARDPUTER_RF_TX_SAMPLE_MS);
      yield();
    }

    _last_tx_wait_ms = (uint16_t)min<uint32_t>(millis() - started, 65535UL);
    if (_last_tx_wait_ms > 0) {
      _tx_window_count++;
    }
  }

public:
  CardputerRfStabilityWrapper(CustomSX1262& radio, mesh::MainBoard& board)
    : CustomSX1262Wrapper(radio, board) { }

  bool startSendRaw(const uint8_t* bytes, int len) override {
    keepRxBoostedGain();
    waitForTxWindow();
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

  uint16_t getLastTxWaitMillis() const { return _last_tx_wait_ms; }
  uint32_t getTxWindowCount() const { return _tx_window_count; }
};
