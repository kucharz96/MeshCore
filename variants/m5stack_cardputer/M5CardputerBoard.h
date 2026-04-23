#pragma once

#include <Wire.h>
#include <Arduino.h>
#include <M5Cardputer.h>
#include "helpers/ESP32Board.h"

#ifndef PIN_VBAT_READ
#define PIN_VBAT_READ 10
#endif

#define BATTERY_SAMPLES 8

class M5CardputerBoard : public ESP32Board {
public:
  void begin() {
    pinMode(46, OUTPUT);
    digitalWrite(46, HIGH);
    delay(100);

    ESP32Board::begin();

    auto cfg = M5.config();
    cfg.clear_display = true;
    cfg.internal_imu = false;
    cfg.internal_rtc = true;
    cfg.internal_spk = false;
    cfg.internal_mic = false;
    M5Cardputer.begin(cfg, true);
    delay(100);
    M5Cardputer.Keyboard.begin();

    M5.In_I2C.writeRegister8(0x43, 0x03, 0x00, 100000);
    delay(10);
    M5.In_I2C.writeRegister8(0x43, 0x01, 0xFF, 100000);
    delay(200);
  }

  uint16_t getBattMilliVolts() override {
  #ifdef PIN_VBAT_READ
    analogReadResolution(12);
    uint32_t raw = 0;
    for (int i = 0; i < BATTERY_SAMPLES; i++) {
      raw += analogReadMilliVolts(PIN_VBAT_READ);
      delay(1);
    }
    raw = raw / BATTERY_SAMPLES;
    return (2 * raw);
  #else
    return 0;
  #endif
  }

  const char* getManufacturerName() const override {
    return "M5Stack Cardputer-Adv";
  }

  void powerOff() override {
    M5.Display.sleep();
  #ifdef PIN_USER_BTN
    enterDeepSleep(0, PIN_USER_BTN);
  #else
    enterDeepSleep(0, -1);
  #endif
  }

  void enterDeepSleep(uint32_t secs, int pin_wake_btn) {
    esp_sleep_enable_ext1_wakeup((1ULL << P_LORA_DIO_1), ESP_EXT1_WAKEUP_ANY_HIGH);

    if (pin_wake_btn >= 0) {
      esp_sleep_enable_ext1_wakeup((1ULL << pin_wake_btn) | (1ULL << P_LORA_DIO_1), ESP_EXT1_WAKEUP_ANY_HIGH);
    }

    if (secs > 0) {
      esp_sleep_enable_timer_wakeup(secs * 1000000ULL);
    }

    esp_deep_sleep_start();
  }
};
