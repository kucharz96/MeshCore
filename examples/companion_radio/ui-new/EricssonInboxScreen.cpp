#include "EricssonInboxScreen.h"

#if defined(WIO_TRACKER_L1)
#include "UITask.h"
#include "EricssonInboxModel.h"
#include <stdio.h>
#include <string.h>

namespace {
constexpr uint8_t kRows = 3;
constexpr size_t kCharsPerLine = 19;
constexpr size_t kCharsPerPage = 2 * kCharsPerLine;

void footer(DisplayDriver& display, const char* left, const char* right) {
  display.setColor(UIColor::secondary_txt);
  display.fillRect(0, 53, display.width(), 1);
  display.drawTextLeftAlign(2, 55, left);
  display.drawTextRightAlign(display.width() - 3, 55, right);
}

void printLine(DisplayDriver& display, const char* source, size_t begin, int y) {
  const size_t length = strlen(source);
  if (begin >= length) return;
  char line[kCharsPerLine + 1];
  size_t n = length - begin;
  if (n > kCharsPerLine) n = kCharsPerLine;
  memcpy(line, source + begin, n);
  line[n] = '\0';
  display.drawTextEllipsized(3, y, display.width() - 6, line);
}
}

void EricssonInboxScreen::open() {
  _task->inbox().selectLatest();
  _details = false;
  _page = 0;
}

int EricssonInboxScreen::render(DisplayDriver& display) {
  EricssonInboxModel& inbox = _task->inbox();
  display.setTextSize(1);
  display.setColor(UIColor::title_bkg);
  display.fillRect(0, 0, display.width(), 12);
  display.setColor(UIColor::title_txt);
  display.drawTextLeftAlign(3, 2, "WIADOMOSCI");

  char counter[14];
  snprintf(counter, sizeof(counter), "%u/%u",
           (unsigned)(inbox.empty() ? 0 : inbox.selected() + 1),
           (unsigned)inbox.count());
  display.drawTextRightAlign(display.width() - 3, 2, counter);

  if (inbox.empty()) {
    display.setColor(UIColor::secondary_txt);
    display.drawTextCentered(display.width() / 2, 27, "Brak wiadomosci");
    footer(display, "Menu", "-");
    return 5000;
  }

  if (_details) {
    const EricssonInboxModel::Entry* entry = inbox.current();
    if (!entry) {
      _details = false;
      footer(display, "Lista", "-");
      return 1000;
    }

    char author[EricssonInboxModel::SenderBytes];
    display.translateUTF8ToBlocks(author, entry->sender, sizeof(author));
    display.setColor(UIColor::primary_txt);
    display.drawTextEllipsized(3, 15, display.width() - 6, author);

    char flattened[EricssonInboxModel::TextBytes];
    display.translateUTF8ToBlocks(flattened, entry->text, sizeof(flattened));
    const size_t length = strlen(flattened);
    size_t offset = static_cast<size_t>(_page) * kCharsPerPage;
    if (offset >= length) {
      _page = 0;  // also handles an entry being overwritten while visible
      offset = 0;
    }
    display.setColor(UIColor::secondary_txt);
    printLine(display, flattened, offset, 28);
    printLine(display, flattened, offset + kCharsPerLine, 40);
    footer(display, "Lista", "Dalej");
    return 5000;
  }

  const uint8_t first = inbox.firstVisible(kRows);
  for (uint8_t row = 0; row < kRows; ++row) {
    const uint8_t index = first + row;
    const auto* entry = inbox.at(index);
    if (!entry) break;
    const int y = 14 + 13 * row;
    const bool selected = index == inbox.selected();

    if (selected) {
      display.setColor(UIColor::primary_txt);
      display.fillRect(0, y, display.width(), 12);
      display.setColor(UIColor::window_bkg);
    } else {
      display.setColor(UIColor::secondary_txt);
    }

    char author[EricssonInboxModel::SenderBytes];
    display.translateUTF8ToBlocks(author, entry->sender, sizeof(author));
    display.drawTextEllipsized(4, y + 2, display.width() - 16, author);
    if (entry->channel) {
      display.drawTextRightAlign(display.width() - 3, y + 2, "#");
    }
  }

  footer(display, "Menu", "Czytaj");
  return 5000;
}

bool EricssonInboxScreen::handleInput(char key) {
  EricssonInboxModel& inbox = _task->inbox();
  switch (static_cast<uint8_t>(key)) {
    case KEY_UP:
    case KEY_PREV:
      if (_details) {
        if (_page > 0) --_page;
      } else {
        inbox.previous();
      }
      return true;
    case KEY_DOWN:
    case KEY_NEXT:
      if (_details) {
        const auto* entry = inbox.current();
        if (entry && (static_cast<size_t>(_page) + 1) * kCharsPerPage < strlen(entry->text)) ++_page;
      } else {
        inbox.next();
      }
      return true;
    case KEY_RIGHT:
    case KEY_ENTER:
      if (!_details) {
        if (!inbox.empty()) {
          _details = true;
          _page = 0;
        }
      } else {
        const auto* entry = inbox.current();
        if (entry && (static_cast<size_t>(_page) + 1) * kCharsPerPage < strlen(entry->text)) ++_page;
      }
      return true;
    case KEY_LEFT:
    case KEY_CANCEL:
      if (_details) {
        _details = false;
        _page = 0;
      } else {
        _task->openEricssonMenu();
      }
      return true;
    default:
      return false;
  }
}
#endif // WIO_TRACKER_L1
