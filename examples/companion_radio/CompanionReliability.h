#pragma once

#include <Arduino.h>

/**
 * Local reliability tracker for Companion Radio.
 *
 * Compatibility rules:
 * - does not change MeshCore packet format
 * - does not change ACK format
 * - does not require repeater or room-server changes
 * - does not change the companion frame protocol
 *
 * This is a safe foundation for later smart direct/flood decisions.
 */
class CompanionReliability {
public:
  enum SendMode : uint8_t {
    SendModeFlood = 0,
    SendModeDirect = 1,
    SendModeDirectCautious = 2,
    SendModeFloodDiscovery = 3
  };

  struct Stats {
    uint8_t pubkey_prefix[6];
    uint8_t score;
    uint8_t fail_streak;
    uint16_t direct_sent;
    uint16_t flood_sent;
    uint16_t ack_ok;
    uint16_t ack_timeout;
    uint16_t rtt_ewma_ms;
    uint32_t last_send_ms;
    uint32_t last_success_ms;
    uint32_t last_fail_ms;
  };

  static const uint8_t DEFAULT_SCORE = 70;
  static const uint8_t DIRECT_SCORE_THRESHOLD = 75;
  static const uint8_t CAUTIOUS_SCORE_THRESHOLD = 50;
  static const uint8_t CIRCUIT_BREAKER_FAILS = 3;
  static const uint8_t ACK_REWARD = 8;
  static const uint8_t TIMEOUT_PENALTY = 15;
  static const uint8_t EWMA_ALPHA_PERCENT = 20;

  CompanionReliability() { clear(); }

  void clear() {
    memset(_entries, 0, sizeof(_entries));
    _next_evict = 0;
  }

  Stats* find(const uint8_t pubkey_prefix[6]) {
    for (uint8_t i = 0; i < MAX_ENTRIES; i++) {
      if (isUsed(_entries[i]) && memcmp(_entries[i].pubkey_prefix, pubkey_prefix, 6) == 0) {
        return &_entries[i];
      }
    }
    return NULL;
  }

  Stats* getOrCreate(const uint8_t pubkey_prefix[6]) {
    Stats* existing = find(pubkey_prefix);
    if (existing) return existing;

    uint8_t idx = findFreeSlot();
    if (idx == 0xFF) {
      idx = _next_evict;
      _next_evict = (_next_evict + 1) % MAX_ENTRIES;
    }

    Stats& s = _entries[idx];
    memset(&s, 0, sizeof(s));
    memcpy(s.pubkey_prefix, pubkey_prefix, 6);
    s.score = DEFAULT_SCORE;
    return &s;
  }

  SendMode chooseSendMode(const uint8_t pubkey_prefix[6], uint8_t out_path_len) {
    if (out_path_len == 0xFF) return SendModeFlood;

    Stats* s = getOrCreate(pubkey_prefix);
    if (!s) return SendModeDirect;

    if (s->fail_streak >= CIRCUIT_BREAKER_FAILS) return SendModeFloodDiscovery;
    if (s->score >= DIRECT_SCORE_THRESHOLD) return SendModeDirect;
    if (s->score >= CAUTIOUS_SCORE_THRESHOLD) return SendModeDirectCautious;
    return SendModeFloodDiscovery;
  }

  void recordSend(const uint8_t pubkey_prefix[6], SendMode mode, uint32_t now_ms) {
    Stats* s = getOrCreate(pubkey_prefix);
    if (!s) return;

    s->last_send_ms = now_ms;
    if (mode == SendModeDirect || mode == SendModeDirectCautious) {
      incrementSat(s->direct_sent);
    } else {
      incrementSat(s->flood_sent);
    }
  }

  void recordAck(const uint8_t pubkey_prefix[6], uint32_t rtt_ms, uint32_t now_ms) {
    Stats* s = getOrCreate(pubkey_prefix);
    if (!s) return;

    incrementSat(s->ack_ok);
    s->fail_streak = 0;
    s->last_success_ms = now_ms;
    s->rtt_ewma_ms = ewmaU16(s->rtt_ewma_ms, clampU32ToU16(rtt_ms));
    s->score = addSat8(s->score, ACK_REWARD);
  }

  void recordTimeout(const uint8_t pubkey_prefix[6], uint32_t now_ms) {
    Stats* s = getOrCreate(pubkey_prefix);
    if (!s) return;

    incrementSat(s->ack_timeout);
    if (s->fail_streak < 0xFF) s->fail_streak++;
    s->last_fail_ms = now_ms;
    s->score = subSat8(s->score, TIMEOUT_PENALTY);
  }

private:
  static const uint8_t MAX_ENTRIES = 32;

  Stats _entries[MAX_ENTRIES];
  uint8_t _next_evict;

  static bool isUsed(const Stats& s) {
    for (uint8_t i = 0; i < 6; i++) {
      if (s.pubkey_prefix[i] != 0) return true;
    }
    return false;
  }

  uint8_t findFreeSlot() const {
    for (uint8_t i = 0; i < MAX_ENTRIES; i++) {
      if (!isUsed(_entries[i])) return i;
    }
    return 0xFF;
  }

  static void incrementSat(uint16_t& v) {
    if (v < 0xFFFF) v++;
  }

  static uint8_t addSat8(uint8_t v, uint8_t delta) {
    return (v > 255 - delta) ? 255 : v + delta;
  }

  static uint8_t subSat8(uint8_t v, uint8_t delta) {
    return (v < delta) ? 0 : v - delta;
  }

  static uint16_t clampU32ToU16(uint32_t v) {
    return v > 0xFFFF ? 0xFFFF : (uint16_t)v;
  }

  static uint16_t ewmaU16(uint16_t old_value, uint16_t sample) {
    if (old_value == 0) return sample;
    uint32_t blended = ((uint32_t)old_value * (100 - EWMA_ALPHA_PERCENT)) +
                       ((uint32_t)sample * EWMA_ALPHA_PERCENT);
    return (uint16_t)(blended / 100);
  }
};
