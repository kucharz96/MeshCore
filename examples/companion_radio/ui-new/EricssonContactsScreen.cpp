#include "EricssonContactsScreen.h"

#if defined(WIO_TRACKER_L1)

#include "UITask.h"
#include "../MyMesh.h"
#include <target.h>
#include <stdio.h>
#include <string.h>

namespace {
constexpr uint8_t kRows = 3;

uint16_t currentContactCount() {
  const int count = the_mesh.getNumContacts();
  return count > 0 ? static_cast<uint16_t>(count) : 0;
}

bool getContactAt(uint16_t displayIndex, ContactInfo& info) {
  if (displayIndex >= currentContactCount()) return false;
  // Official MeshCore reserves the first MAX_ANON_CONTACTS slots for
  // transient peers. Never expose them in the user's address book.
  return the_mesh.getContactByIdx(MAX_ANON_CONTACTS + displayIndex, info);
}

const char* contactType(uint8_t type) {
  switch (type) {
    case ADV_TYPE_CHAT: return "Companion";
    case ADV_TYPE_REPEATER: return "Repeater";
    case ADV_TYPE_ROOM: return "Room";
    default: return "Inny";
  }
}

void safeName(const ContactInfo& info, char (&name)[33]) {
  memcpy(name, info.name, sizeof(info.name));
  name[sizeof(info.name)] = '\0';
}

void drawFooter(DisplayDriver& display, const char* left, const char* right) {
  display.setColor(UIColor::secondary_txt);
  display.fillRect(0, 53, display.width(), 1);
  display.drawTextLeftAlign(2, 55, left);
  display.drawTextRightAlign(display.width() - 3, 55, right);
}
}

void EricssonContactsScreen::open() {
  _pager.reset(currentContactCount());
  _details = false;
}

int EricssonContactsScreen::render(DisplayDriver& display) {
  const uint16_t oldCount = _pager.count();
  _pager.setCount(currentContactCount());
  if (_details && oldCount != _pager.count()) _details = false;

  display.setTextSize(1);
  display.setColor(UIColor::title_bkg);
  display.fillRect(0, 0, display.width(), 12);
  display.setColor(UIColor::title_txt);
  display.drawTextLeftAlign(3, 2, "KONTAKTY");

  char status[16];
  snprintf(status, sizeof(status), "%u/%u",
           static_cast<unsigned>(_pager.empty() ? 0 : _pager.selected() + 1),
           static_cast<unsigned>(_pager.count()));
  display.drawTextRightAlign(display.width() - 3, 2, status);

  if (_pager.empty()) {
    display.setColor(UIColor::secondary_txt);
    display.drawTextCentered(display.width() / 2, 28, "Brak kontaktow");
    drawFooter(display, "Menu", "-");
    return 5000;
  }

  if (_details) {
    ContactInfo info;
    if (!getContactAt(_pager.selected(), info)) {
      _details = false;
      display.drawTextLeftAlign(3, 23, "Kontakt usuniety");
      drawFooter(display, "Lista", "-");
      return 1000;
    }

    char raw[33], name[33];
    safeName(info, raw);
    display.translateUTF8ToBlocks(name, raw, sizeof(name));
    display.setColor(UIColor::primary_txt);
    display.drawTextEllipsized(3, 16, display.width() - 6, name);
    display.setColor(UIColor::secondary_txt);

    char type[40];
    snprintf(type, sizeof(type), "Typ: %s", contactType(info.type));
    display.drawTextLeftAlign(3, 28, type);

    char route[40];
    if (info.out_path_len == OUT_PATH_UNKNOWN) {
      snprintf(route, sizeof(route), "Trasa: nieznana%s", info.isFav() ? " *" : "");
    } else {
      snprintf(route, sizeof(route), "Trasa: %u B%s",
               static_cast<unsigned>(info.out_path_len),
               info.isFav() ? " *" : "");
    }
    display.drawTextLeftAlign(3, 40, route);
    drawFooter(display, "Lista", "-");
    return 5000;
  }

  const uint16_t first = _pager.firstVisible(kRows);
  for (uint8_t row = 0; row < kRows; ++row) {
    const uint16_t index = first + row;
    if (index >= _pager.count()) break;

    const int y = 14 + row * 13;
    const bool selected = index == _pager.selected();
    if (selected) {
      display.setColor(UIColor::primary_txt);
      display.fillRect(0, y, display.width(), 12);
      display.setColor(UIColor::window_bkg);
    } else {
      display.setColor(UIColor::secondary_txt);
    }

    ContactInfo info;
    if (getContactAt(index, info)) {
      char raw[33], name[33];
      safeName(info, raw);
      display.translateUTF8ToBlocks(name, raw, sizeof(name));
      display.drawTextEllipsized(4, y + 2, display.width() - 15, name);
      if (info.isFav()) {
        display.drawTextRightAlign(display.width() - 3, y + 2, "*");
      }
    } else {
      display.drawTextLeftAlign(4, y + 2, "(niedostepny)");
    }
  }

  drawFooter(display, "Menu", "Pokaz");
  return 5000;
}

bool EricssonContactsScreen::handleInput(char c) {
  switch (static_cast<uint8_t>(c)) {
    case KEY_UP:
    case KEY_PREV:
      if (!_details) _pager.previous();
      return true;
    case KEY_DOWN:
    case KEY_NEXT:
      if (!_details) _pager.next();
      return true;
    case KEY_RIGHT:
    case KEY_ENTER:
      if (!_details && !_pager.empty()) _details = true;
      return true;
    case KEY_LEFT:
    case KEY_CANCEL:
      if (_details) {
        _details = false;
      } else {
        _task->openEricssonMenu();
      }
      return true;
    default:
      return false;
  }
}

#endif
