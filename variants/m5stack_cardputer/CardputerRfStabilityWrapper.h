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

#ifndef CARDPUTER_RF_RX_CAPTURE_GUARD_MS
#define CARDPUTER_RF_RX_CAPTURE_GUARD_MS 550UL
#endif

class CardputerRfStabilityWrapper : public CustomSX1262Wrapper {
  uint32_t _last_rx_boost_check_ms = 0;
  uint32_t _last_rx_packet_ms = 0;
  uint16_t _last_tx_wait_ms = 0;
  uint16_t _last_rx_guard_wait_ms = 0;
  uint32_t _tx_window_count = 0;
  uint32_t _rx_guard_count = 0;

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

  uint8_t payloadTypeFromRaw(const uint8_t* bytes, int len) const {
    if (bytes == nullptr || len <= 0) {
      return 0xFF;
    }
    return (bytes[0] >> PH_TYPE_SHIFT) & PH_TYPE_MASK;
  }

  bool shouldSkipRxCaptureGuard(uint8_t payload_type) const {
    // Keep ACK/path/response packets fast. Delaying those would hurt the very
    // return traffic that the guard is trying to protect.
    return payload_type == PAYLOAD_TYPE_ACK ||
           payload_type == PAYLOAD_TYPE_PATH ||
           payload_type == PAYLOAD_TYPE_RESPONSE;
  }

  void waitAfterFreshRxIfUseful(uint8_t payload_type) {
    _last_rx_guard_wait_ms = 0;
    if (_last_rx_packet_ms == 0 || shouldSkipRxCaptureGuard(payload_type)) {
      return;
    }

    uint32_t now = millis();
    uint32_t age = now - _last_rx_packet_ms;
    if (age >= CARDPUTER_RF_RX_CAPTURE_GUARD_MS) {
      return;
    }

    uint32_t wait_ms = CARDPUTER_RF_RX_CAPTURE_GUARD_MS - age;
    uint32_t started = now;

    while ((uint32_t)(millis() - started) < wait_ms) {
      // If another packet is already being received, stay out of TX a little longer,
      // but never beyond the bounded guard window.
      delay(CARDPUTER_RF_TX_SAMPLE_MS);
      yield();
    }

    _last_rx_guard_wait_ms = (uint16_t)min<uint32_t>(millis() - started, 65535UL);
    if (_last_rx_guard_wait_ms > 0) {
      _rx_guard_count++;
    }
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
    uint8_t payload_type = payloadTypeFromRaw(bytes, len);
    keepRxBoostedGain();
    waitAfterFreshRxIfUseful(payload_type);
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
      _last_rx_packet_ms = millis();
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
  uint16_t getLastRxGuardWaitMillis() const { return _last_rx_guard_wait_ms; }
  uint32_t getRxGuardCount() const { return _rx_guard_count; }
};
