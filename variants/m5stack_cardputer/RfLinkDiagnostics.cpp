#include <Arduino.h>
#include "target.h"

#ifdef CARDPUTER_RF_DIAG_OVERLAY

namespace {
struct RfDiagState {
  uint32_t lastRecvCount = 0;
  uint32_t lastSentCount = 0;
  uint32_t lastRxMillis = 0;
  uint32_t bestScoreMillis = 0;
  uint8_t bestScore = 0;
  float lastRssi = 0.0f;
  float lastSnr = 0.0f;
};

RfDiagState rf_diag;

static uint8_t clampScore(int value) {
  if (value < 0) return 0;
  if (value > 100) return 100;
  return (uint8_t)value;
}

static int scoreFromSnr(float snr) {
  // LoRa can still decode negative SNR, but field usability improves quickly above -10 dB.
  if (snr <= -20.0f) return 0;
  if (snr >= 5.0f) return 35;
  return (int)((snr + 20.0f) * 35.0f / 25.0f);
}

static int scoreFromRssi(float rssi) {
  // Practical handheld range: around -125 dBm weak, -105 dBm usable, > -95 dBm good.
  if (rssi <= -130.0f) return 0;
  if (rssi >= -95.0f) return 25;
  return (int)((rssi + 130.0f) * 25.0f / 35.0f);
}

static int scoreFromFreshness(uint32_t age_ms) {
  if (rf_diag.lastRxMillis == 0) return 0;
  if (age_ms <= 5000UL) return 25;
  if (age_ms >= 120000UL) return 0;
  return (int)(25 - ((age_ms - 5000UL) * 25UL / 115000UL));
}

static int scoreFromBattery(uint16_t mv) {
  if (mv == 0) return 0;
  if (mv >= 3900) return 15;
  if (mv <= 3400) return 0;
  return (int)((mv - 3400) * 15 / 500);
}

static const char* hintFor(uint8_t score, uint16_t batt_mv, uint32_t age_ms) {
  if (batt_mv > 0 && batt_mv < 3550) return "BAT";
  if (rf_diag.lastRxMillis == 0 || age_ms > 120000UL) return "MOVE";
  if (score >= 70 && age_ms <= 15000UL) return "SEND";
  if (score >= 45) return "HOLD";
  return "MOVE";
}

static void formatAge(char* dest, size_t len, uint32_t age_ms) {
  if (rf_diag.lastRxMillis == 0) {
    snprintf(dest, len, "--");
    return;
  }

  uint32_t secs = age_ms / 1000UL;
  if (secs < 60UL) {
    snprintf(dest, len, "%lus", (unsigned long)secs);
  } else if (secs < 3600UL) {
    snprintf(dest, len, "%lum", (unsigned long)(secs / 60UL));
  } else {
    snprintf(dest, len, "%luh", (unsigned long)(secs / 3600UL));
  }
}
}

void cardputerDrawRfOverlay(M5CardputerDisplay& disp) {
  uint32_t now = millis();
  uint32_t recvCount = radio_driver.getPacketsRecv();
  uint32_t sentCount = radio_driver.getPacketsSent();

  if (recvCount != rf_diag.lastRecvCount) {
    rf_diag.lastRecvCount = recvCount;
    rf_diag.lastRxMillis = now;
    rf_diag.lastRssi = radio_driver.getLastRSSI();
    rf_diag.lastSnr = radio_driver.getLastSNR();
  }
  rf_diag.lastSentCount = sentCount;

  uint16_t batt_mv = board.getBattMilliVolts();
  uint32_t age_ms = rf_diag.lastRxMillis == 0 ? 0xFFFFFFFFUL : now - rf_diag.lastRxMillis;

  int score = 0;
  score += scoreFromSnr(rf_diag.lastSnr);
  score += scoreFromRssi(rf_diag.lastRssi);
  score += scoreFromFreshness(age_ms);
  score += scoreFromBattery(batt_mv);
  uint8_t linkScore = clampScore(score);

  if (linkScore > rf_diag.bestScore || (now - rf_diag.bestScoreMillis) > 60000UL) {
    rf_diag.bestScore = linkScore;
    rf_diag.bestScoreMillis = now;
  }

  char age[8];
  formatAge(age, sizeof(age), age_ms);

  char line1[42];
  char line2[42];
  const char* hint = hintFor(linkScore, batt_mv, age_ms);

  snprintf(line1, sizeof(line1), "RF:%03u %s BAT:%u.%02uV",
           (unsigned)linkScore,
           hint,
           (unsigned)(batt_mv / 1000),
           (unsigned)((batt_mv % 1000) / 10));

  snprintf(line2, sizeof(line2), "S:%+.1f R:%d RX:%s B:%03u",
           rf_diag.lastSnr,
           (int)rf_diag.lastRssi,
           age,
           (unsigned)rf_diag.bestScore);

  const int overlayY = disp.height() - 22;
  disp.setTextSize(1);
  disp.setColor(DisplayDriver::DARK);
  disp.fillRect(0, overlayY - 1, disp.width(), 23);
  disp.setColor(linkScore >= 70 ? DisplayDriver::GREEN : (linkScore >= 45 ? DisplayDriver::YELLOW : DisplayDriver::RED));
  disp.setCursor(0, overlayY);
  disp.print(line1);
  disp.setCursor(0, overlayY + 11);
  disp.print(line2);
}

#endif
