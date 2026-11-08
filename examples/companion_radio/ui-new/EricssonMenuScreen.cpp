#include "EricssonMenuScreen.h"

#if defined(WIO_TRACKER_L1)

#include "UITask.h"
#include "../MyMesh.h"
#include <target.h>
#include <stdio.h>

namespace {
constexpr uint8_t kVisibleRows = 3;
constexpr uint8_t kMenuCount = 8;

const char* const kEntries[kMenuCount] = {
  "Wiadomosci", "Kontakty", "Radio", "Bluetooth",
  "GPS", "Rozgloszenie", "Bateria", "Powrot"
};
}

int EricssonMenuScreen::render(DisplayDriver& display) {
  const int width = display.width();
  display.setTextSize(1);

  // Classic compact mobile-phone header, three-item scrolling list and
  // two fixed softkey labels. Nothing is redrawn while the OLED is off.
  display.setColor(UIColor::title_bkg);
  display.fillRect(0, 0, width, 12);
  display.setColor(UIColor::title_txt);
  display.drawTextLeftAlign(3, 2, "MESHCORE X");

  char page[12];
  snprintf(page, sizeof(page), "%u/%u",
           (unsigned)(_menu.selected() + 1), (unsigned)_menu.count());
  display.drawTextRightAlign(width - 3, 2, page);

  const uint8_t first = _menu.firstVisible(kVisibleRows);
  for (uint8_t row = 0; row < kVisibleRows; ++row) {
    const uint8_t index = first + row;
    if (index >= kMenuCount) break;

    const int y = 14 + 13 * row;
    const bool selected = index == _menu.selected();
    if (selected) {
      display.setColor(UIColor::primary_txt);
      display.fillRect(0, y, width, 12);
      display.setColor(UIColor::window_bkg);
    } else {
      display.setColor(UIColor::secondary_txt);
    }
    display.drawTextEllipsized(5, y + 2, width - 12, kEntries[index]);
  }

  display.setColor(UIColor::secondary_txt);
  display.fillRect(0, 53, width, 1);
  display.drawTextLeftAlign(2, 55, "Cofnij");
  display.drawTextRightAlign(width - 3, 55, "OK");
  return 5000;
}

bool EricssonMenuScreen::handleInput(char c) {
  switch (static_cast<unsigned char>(c)) {
    case KEY_UP:
    case KEY_PREV:
      _menu.up();
      return true;
    case KEY_DOWN:
    case KEY_NEXT:
      _menu.down();
      return true;
    case KEY_LEFT:
    case KEY_CANCEL:
      _task->gotoHomeScreen();
      return true;
    case KEY_RIGHT:
    case KEY_ENTER:
      activate();
      return true;
    default:
      return false;
  }
}

void EricssonMenuScreen::activate() {
  char msg[64];
  switch (_menu.selected()) {
    case 0:
      snprintf(msg, sizeof(msg), "Nieprzeczytane: %d", _task->getMsgCount());
      _task->showAlert(msg, 1600);
      break;
    case 1:
      snprintf(msg, sizeof(msg), "Kontakty: %lu", (unsigned long)the_mesh.getNumContacts());
      _task->showAlert(msg, 1600);
      break;
    case 2:
      snprintf(msg, sizeof(msg), "RX:%lu TX:%lu",
               (unsigned long)radio_driver.getPacketsRecv(),
               (unsigned long)radio_driver.getPacketsSent());
      _task->showAlert(msg, 2000);
      break;
    case 3:
      if (_task->isBluetoothEnabled()) _task->disableBluetooth();
      else _task->enableBluetooth();
      _task->showAlert(_task->isBluetoothEnabled() ? "Bluetooth: ON" : "Bluetooth: OFF", 1600);
      break;
    case 4:
#if ENV_INCLUDE_GPS == 1
      _task->toggleGPS();
      _task->showAlert(_task->getGPSState() ? "GPS: ON" : "GPS: OFF", 1600);
#else
      _task->showAlert("GPS niedostepny", 1600);
#endif
      break;
    case 5:
      _task->showAlert(the_mesh.advert() ? "Rozgloszenie OK" : "Blad rozgloszenia", 1600);
      break;
    case 6:
      snprintf(msg, sizeof(msg), "Bateria: %u mV", (unsigned)board.getBattMilliVolts());
      _task->showAlert(msg, 1600);
      break;
    case 7:
      _task->gotoHomeScreen();
      break;
    default:
      break;
  }
}

#endif // WIO_TRACKER_L1
