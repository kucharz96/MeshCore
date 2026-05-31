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
 * Design goals:
 * - prefer stable direct paths when they work
 * - quickly stop trusting repeated failing direct paths
 * - avoid route flapping with hysteresis
 * - avoid excessive discovery/flooding with cooldowns
 * - learn from passive received-link quality without increasing TX power
 * - keep the implementation lightweight for embedded targets
 */
class CompanionReliability {
public:
  enum SendMode : uint8_t {
    SendModeFlood = 0,
    SendModeDirect = 1,
    SendModeDirectCautious = 2,
    SendModeFloodDiscovery = 3
  };

  enum Health : uint8_t {
    HealthUnknown = 0,
    HealthGood = 1,
    HealthWeak = 2,
    HealthBad = 3,
    HealthCircuitOpen = 4
  };

  struct Stats {
    uint8_t pubkey_prefix[6];

    uint8_t score;               // 0..100 final local reliability score
    uint8_t delivery_ewma;       // 0..100 ACK-derived delivery probability
    uint8_t fail_streak;         // consecutive timeout/retry observations
    uint8_t selected_direct_streak;
    uint8_t last_send_mode;

    uint16_t direct_sent;
    uint16_t flood_sent;
    uint16_t ack_ok;
    uint16_t ack_timeout;
    uint16_t heard_count;
    uint16_t rtt_ewma_ms;

    int8_t snr_ewma_x4;          // SNR * 4, as used by the companion protocol
    int8_t rssi_ewma_dbm;        // coarse RSSI EWMA, signed dBm

    uint32_t last_send_ms;
    uint32_t last_success_ms;
    uint32_t last_fail_ms;
    uint32_t last_heard_ms;
    uint32_t last_flood_discovery_ms;
    uint32_t last_score_update_ms;
  };

  struct Snapshot {
    Stats stats;
    uint8_t out_path_len;
    Health health;
    SendMode recommended_mode;
  };

  static const uint8_t DEFAULT_SCORE = 70;
  static const uint8_t DEFAULT_DELIVERY_EWMA = 70;
  static const uint8_t DIRECT_SCORE_THRESHOLD = 76;
  static const uint8_t CAUTIOUS_SCORE_THRESHOLD = 52;
  static const uint8_t FLOOD_DISCOVERY_SCORE_THRESHOLD = 42;
  static const uint8_t CIRCUIT_BREAKER_FAILS = 3;
  static const uint8_t ACK_REWARD = 7;
  static const uint8_t TIMEOUT_PENALTY = 16;
  static const uint8_t HEARD_REWARD = 2;
  static const uint8_t EWMA_ALPHA_PERCENT = 20;

  static const uint32_t STALE_PATH_MS = 30UL * 60UL * 1000UL;
  static const uint32_t VERY_STALE_PATH_MS = 2UL * 60UL * 60UL * 1000UL;
  static const uint32_t STALE_HEARD_MS = 20UL * 60UL * 1000UL;
  static const uint32_t FLOOD_DISCOVERY_COOLDOWN_MS = 90UL * 1000UL;

  CompanionReliability() { clear(); }

  void clear() {
    memset(_entries, 0, sizeof(_entries));
    _next_evict = 0;
    _total_sends = 0;
  }

  Stats* find(const uint8_t pubkey_prefix[6]) {
    for (uint8_t i = 0; i < MAX_ENTRIES; i++) {
      if (isUsed(_entries[i]) && memcmp(_entries[i].pubkey_prefix, pubkey_prefix, 6) == 0) {
        return &_entries[i];
      }
    }
    return NULL;
  }

  const Stats* findConst(const uint8_t pubkey_prefix[6]) const {
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
      idx = chooseEvictionSlot();
    }

    Stats& s = _entries[idx];
    memset(&s, 0, sizeof(s));
    memcpy(s.pubkey_prefix, pubkey_prefix, 6);
    s.score = DEFAULT_SCORE;
    s.delivery_ewma = DEFAULT_DELIVERY_EWMA;
    s.last_send_mode = SendModeFlood;
    return &s;
  }

  /**
   * Decide how the companion should send to this contact using only local state.
   * It intentionally returns modes that can be mapped to official MeshCore send paths.
   */
  SendMode chooseSendMode(const uint8_t pubkey_prefix[6], uint8_t out_path_len, uint32_t now_ms = 0) {
    if (out_path_len == 0xFF) return SendModeFlood;

    Stats* s = getOrCreate(pubkey_prefix);
    if (!s) return SendModeDirect;

    recomputeScore(*s, out_path_len, now_ms);

    SendMode mode = recommendModeFromStats(*s, out_path_len, now_ms);
    if (mode == SendModeDirect || mode == SendModeDirectCautious) {
      s->selected_direct_streak = addSat8(s->selected_direct_streak, 1);
    } else if (mode == SendModeFloodDiscovery) {
      s->selected_direct_streak = 0;
      s->last_flood_discovery_ms = now_ms;
    }
    return mode;
  }

  void recordSend(const uint8_t pubkey_prefix[6], SendMode mode, uint32_t now_ms) {
    Stats* s = getOrCreate(pubkey_prefix);
    if (!s) return;

    incrementSat(_total_sends);
    s->last_send_ms = now_ms;
    s->last_send_mode = mode;
    if (mode == SendModeDirect || mode == SendModeDirectCautious) {
      incrementSat(s->direct_sent);
    } else {
      incrementSat(s->flood_sent);
      s->last_flood_discovery_ms = now_ms;
    }
  }

  void recordAck(const uint8_t pubkey_prefix[6], uint32_t rtt_ms, uint32_t now_ms) {
    Stats* s = getOrCreate(pubkey_prefix);
    if (!s) return;

    incrementSat(s->ack_ok);
    s->fail_streak = 0;
    s->last_success_ms = now_ms;
    s->last_score_update_ms = now_ms;
    s->rtt_ewma_ms = ewmaU16(s->rtt_ewma_ms, clampU32ToU16(rtt_ms));
    s->delivery_ewma = ewmaU8(s->delivery_ewma, 100);
    s->score = addSat8(s->score, ACK_REWARD);
  }

  void recordTimeout(const uint8_t pubkey_prefix[6], uint32_t now_ms) {
    Stats* s = getOrCreate(pubkey_prefix);
    if (!s) return;

    incrementSat(s->ack_timeout);
    if (s->fail_streak < 0xFF) s->fail_streak++;
    s->last_fail_ms = now_ms;
    s->last_score_update_ms = now_ms;
    s->delivery_ewma = ewmaU8(s->delivery_ewma, 0);
    s->score = subSat8(s->score, TIMEOUT_PENALTY);
  }

  /**
   * Passive link-quality learning. This improves range quality without raising TX power:
   * a contact repeatedly heard with usable SNR/RSSI becomes more trusted; very weak
   * receptions make the companion more conservative.
   */
  void recordHeard(const uint8_t pubkey_prefix[6], int8_t snr_x4, int8_t rssi_dbm, uint32_t now_ms) {
    Stats* s = getOrCreate(pubkey_prefix);
    if (!s) return;

    incrementSat(s->heard_count);
    s->last_heard_ms = now_ms;
    s->last_score_update_ms = now_ms;
    s->snr_ewma_x4 = ewmaI8(s->snr_ewma_x4, snr_x4);
    s->rssi_ewma_dbm = ewmaI8(s->rssi_ewma_dbm, rssi_dbm);

    if (snr_x4 >= 16) {           // >= +4 dB
      s->score = addSat8(s->score, HEARD_REWARD);
    } else if (snr_x4 < -24) {    // < -6 dB
      s->score = subSat8(s->score, 4);
    }
  }

  uint8_t getScore(const uint8_t pubkey_prefix[6], uint8_t out_path_len, uint32_t now_ms = 0) {
    Stats* s = getOrCreate(pubkey_prefix);
    if (!s) return DEFAULT_SCORE;
    recomputeScore(*s, out_path_len, now_ms);
    return s->score;
  }

  uint8_t getFailStreak(const uint8_t pubkey_prefix[6]) const {
    const Stats* s = findConst(pubkey_prefix);
    return s ? s->fail_streak : 0;
  }

  Health getHealth(const uint8_t pubkey_prefix[6], uint8_t out_path_len, uint32_t now_ms = 0) {
    const Stats* existing = findConst(pubkey_prefix);
    if (!existing) return out_path_len == 0xFF ? HealthUnknown : HealthWeak;

    Stats local = *existing;
    recomputeScore(local, out_path_len, now_ms);
    if (local.fail_streak >= CIRCUIT_BREAKER_FAILS) return HealthCircuitOpen;
    if (local.score >= DIRECT_SCORE_THRESHOLD) return HealthGood;
    if (local.score >= CAUTIOUS_SCORE_THRESHOLD) return HealthWeak;
    return HealthBad;
  }

  bool getSnapshot(const uint8_t pubkey_prefix[6], uint8_t out_path_len, uint32_t now_ms, Snapshot& dest) {
    const Stats* existing = findConst(pubkey_prefix);
    if (!existing) return false;

    dest.stats = *existing;
    dest.out_path_len = out_path_len;
    recomputeScore(dest.stats, out_path_len, now_ms);
    dest.health = healthFromStats(dest.stats);
    dest.recommended_mode = recommendModeFromStats(dest.stats, out_path_len, now_ms);
    return true;
  }

  uint8_t getEntryCount() const {
    uint8_t count = 0;
    for (uint8_t i = 0; i < MAX_ENTRIES; i++) {
      if (isUsed(_entries[i])) count++;
    }
    return count;
  }

  uint16_t getTotalSends() const {
    return _total_sends;
  }

  static const char* sendModeName(SendMode mode) {
    switch (mode) {
      case SendModeFlood: return "flood";
      case SendModeDirect: return "direct";
      case SendModeDirectCautious: return "direct_cautious";
      case SendModeFloodDiscovery: return "flood_discovery";
      default: return "unknown";
    }
  }

  static const char* healthName(Health health) {
    switch (health) {
      case HealthGood: return "good";
      case HealthWeak: return "weak";
      case HealthBad: return "bad";
      case HealthCircuitOpen: return "circuit_open";
      case HealthUnknown:
      default: return "unknown";
    }
  }

private:
  static const uint8_t MAX_ENTRIES = 32;

  Stats _entries[MAX_ENTRIES];
  uint8_t _next_evict;
  uint16_t _total_sends;

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

  uint8_t chooseEvictionSlot() {
    uint8_t idx = _next_evict;
    uint32_t oldest_activity = 0xFFFFFFFF;

    for (uint8_t i = 0; i < MAX_ENTRIES; i++) {
      uint32_t activity = _entries[i].last_send_ms;
      if (_entries[i].last_success_ms > activity) activity = _entries[i].last_success_ms;
      if (_entries[i].last_fail_ms > activity) activity = _entries[i].last_fail_ms;
      if (_entries[i].last_heard_ms > activity) activity = _entries[i].last_heard_ms;
      if (activity < oldest_activity && _entries[i].fail_streak > 0) {
        oldest_activity = activity;
        idx = i;
      }
    }

    _next_evict = (idx + 1) % MAX_ENTRIES;
    return idx;
  }

  static Health healthFromStats(const Stats& s) {
    if (s.fail_streak >= CIRCUIT_BREAKER_FAILS) return HealthCircuitOpen;
    if (s.score >= DIRECT_SCORE_THRESHOLD) return HealthGood;
    if (s.score >= CAUTIOUS_SCORE_THRESHOLD) return HealthWeak;
    return HealthBad;
  }

  static SendMode recommendModeFromStats(const Stats& s, uint8_t out_path_len, uint32_t now_ms) {
    if (out_path_len == 0xFF) return SendModeFlood;
    if (s.fail_streak >= CIRCUIT_BREAKER_FAILS) {
      return canFloodDiscover(s, now_ms) ? SendModeFloodDiscovery : SendModeDirectCautious;
    }
    if (s.score >= DIRECT_SCORE_THRESHOLD) return SendModeDirect;
    if (s.score >= CAUTIOUS_SCORE_THRESHOLD) return SendModeDirectCautious;
    if (s.score <= FLOOD_DISCOVERY_SCORE_THRESHOLD && canFloodDiscover(s, now_ms)) return SendModeFloodDiscovery;
    return SendModeDirectCautious;
  }

  void recomputeScore(Stats& s, uint8_t out_path_len, uint32_t now_ms) {
    uint8_t score = s.delivery_ewma;

    // ETX-like penalty: lower delivery probability means more expected transmissions.
    // Kept integer-only and conservative for embedded builds.
    if (s.delivery_ewma < 30) score = subSat8(score, 22);
    else if (s.delivery_ewma < 50) score = subSat8(score, 14);
    else if (s.delivery_ewma < 70) score = subSat8(score, 6);

    // More path hashes mean more hops and larger packets. Penalise gently.
    uint8_t hop_count = out_path_len & 0x3F;
    if (hop_count > 1) {
      uint8_t hop_penalty = (hop_count - 1) * 3;
      if (hop_penalty > 18) hop_penalty = 18;
      score = subSat8(score, hop_penalty);
    }

    // Circuit breaker penalty for consecutive failed attempts.
    if (s.fail_streak > 0) {
      uint8_t fail_penalty = s.fail_streak * 13;
      if (fail_penalty > 45) fail_penalty = 45;
      score = subSat8(score, fail_penalty);
    }

    // Passive link-quality scoring. Strong heard packets legalistically improve
    // usable range by improving route confidence, not by raising TX power.
    if (s.heard_count > 0) {
      if (s.snr_ewma_x4 >= 16) score = addSat8(score, 6);        // >= +4 dB
      else if (s.snr_ewma_x4 >= 0) score = addSat8(score, 2);    // 0..+4 dB
      else if (s.snr_ewma_x4 < -32) score = subSat8(score, 10);  // < -8 dB
      else if (s.snr_ewma_x4 < -16) score = subSat8(score, 5);   // < -4 dB
    }

    // Age decay: stale paths should not be trusted forever.
    if (now_ms && s.last_success_ms) {
      uint32_t age = now_ms - s.last_success_ms;
      if (age > VERY_STALE_PATH_MS) score = subSat8(score, 22);
      else if (age > STALE_PATH_MS) score = subSat8(score, 10);
    }
    if (now_ms && s.last_heard_ms) {
      uint32_t heard_age = now_ms - s.last_heard_ms;
      if (heard_age > STALE_HEARD_MS) score = subSat8(score, 6);
    }

    // Simple UCB-like exploration bonus. Low-sample paths should not be starved forever.
    uint16_t samples = s.direct_sent + s.flood_sent + s.ack_ok + s.ack_timeout;
    if (samples < 3 && s.fail_streak == 0) score = addSat8(score, 6);

    // Hysteresis: after repeated direct selections, avoid flapping unless score is clearly weak.
    if (s.selected_direct_streak >= 3 && score >= CAUTIOUS_SCORE_THRESHOLD) {
      score = addSat8(score, 4);
    }

    s.score = score;
  }

  static bool canFloodDiscover(const Stats& s, uint32_t now_ms) {
    if (!now_ms || s.last_flood_discovery_ms == 0) return true;
    return now_ms - s.last_flood_discovery_ms >= FLOOD_DISCOVERY_COOLDOWN_MS;
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

  static uint8_t ewmaU8(uint8_t old_value, uint8_t sample) {
    uint16_t blended = ((uint16_t)old_value * (100 - EWMA_ALPHA_PERCENT)) +
                       ((uint16_t)sample * EWMA_ALPHA_PERCENT);
    return (uint8_t)(blended / 100);
  }

  static int8_t ewmaI8(int8_t old_value, int8_t sample) {
    if (old_value == 0) return sample;
    int16_t blended = ((int16_t)old_value * (100 - EWMA_ALPHA_PERCENT)) +
                      ((int16_t)sample * EWMA_ALPHA_PERCENT);
    return (int8_t)(blended / 100);
  }
};
