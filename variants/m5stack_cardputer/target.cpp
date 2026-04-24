#include <Arduino.h>
#include <stdlib.h>
#include "target.h"
#include "../../examples/companion_radio/NodePrefs.h"

M5CardputerBoard board;

static SPIClass spi;
RADIO_CLASS radio = new Module(P_LORA_NSS, P_LORA_DIO_1, P_LORA_RESET, P_LORA_BUSY, spi, SPISettings());
WRAPPER_CLASS radio_driver(radio, board);

ESP32RTCClock fallback_clock;
AutoDiscoverRTCClock rtc_clock(fallback_clock);

#ifdef HAS_GPS
static MicroNMEALocationProvider location_provider(Serial1, &rtc_clock);
CardputerSensorManager sensors(location_provider);

bool CardputerSensorManager::begin() {
  gps_active = false;
  return true;
}

bool CardputerSensorManager::querySensors(uint8_t requester_permissions, CayenneLPP& telemetry) {
  if ((requester_permissions & TELEM_PERM_LOCATION) && gps_active) {
    telemetry.addGPS(TELEM_CHANNEL_SELF, node_lat, node_lon, node_altitude);
  }
  return true;
}

void CardputerSensorManager::loop() {
  static unsigned long next_gps_update = 0;

  if (!gps_active) {
    return;
  }

  _location->loop();

  if (millis() > next_gps_update) {
    if (_location->isValid()) {
      node_lat = ((double)_location->getLatitude()) / 1000000.;
      node_lon = ((double)_location->getLongitude()) / 1000000.;
      node_altitude = ((double)_location->getAltitude()) / 1000.0;
    }
    next_gps_update = millis() + (gps_interval_secs * 1000UL);
  }
}

int CardputerSensorManager::getNumSettings() const {
  return 2;
}

const char* CardputerSensorManager::getSettingName(int i) const {
  switch (i) {
    case 0: return "gps";
    case 1: return "gps_interval";
    default: return NULL;
  }
}

const char* CardputerSensorManager::getSettingValue(int i) const {
  switch (i) {
    case 0:
      return gps_active ? "1" : "0";
    case 1:
      snprintf(_gps_interval_buf, sizeof(_gps_interval_buf), "%lu", (unsigned long)gps_interval_secs);
      return _gps_interval_buf;
    default:
      return NULL;
  }
}

bool CardputerSensorManager::setSettingValue(const char* name, const char* value) {
  if (strcmp(name, "gps") == 0) {
    bool should_enable = (strcmp(value, "0") != 0);
    if (should_enable && !gps_active) {
      Serial1.setPins(PIN_GPS_TX, PIN_GPS_RX);
      Serial1.begin(GPS_BAUD_RATE);
      _location->begin();
      _location->reset();
      _location->syncTime();
      gps_active = true;
    } else if (!should_enable && gps_active) {
      _location->stop();
      Serial1.end();
      gps_active = false;
    }
    return true;
  }

  if (strcmp(name, "gps_interval") == 0) {
    unsigned long parsed = strtoul(value, NULL, 10);
    gps_interval_secs = parsed > 0 ? parsed : 180;
    return true;
  }

  return false;
}
#else
SensorManager sensors;
#endif

#ifdef DISPLAY_CLASS
DISPLAY_CLASS display;
MomentaryButton user_btn(PIN_USER_BTN, 1000, true);
#endif

#ifndef LORA_CR
  #define LORA_CR 5
#endif

bool radio_init() {
  fallback_clock.begin();
  rtc_clock.begin(Wire);

  pinMode(P_LORA_RESET, OUTPUT);
  digitalWrite(P_LORA_RESET, LOW);
  delay(10);
  digitalWrite(P_LORA_RESET, HIGH);
  delay(100);

  bool init_result = radio.std_init(&spi);
  return init_result;
}

uint32_t radio_get_rng_seed() {
  return radio.random(0x7FFFFFFF);
}

void radio_set_params(float freq, float bw, uint8_t sf, uint8_t cr) {
  radio.setFrequency(freq);
  radio.setSpreadingFactor(sf);
  radio.setBandwidth(bw);
  radio.setCodingRate(cr);
}

void radio_set_tx_power(uint8_t dbm) {
  radio.setOutputPower(dbm);
}

mesh::LocalIdentity radio_new_identity() {
  RadioNoiseListener rng(radio);
  return mesh::LocalIdentity(&rng);
}
