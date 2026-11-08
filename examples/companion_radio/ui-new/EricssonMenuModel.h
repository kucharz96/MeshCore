#pragma once

#include <stdint.h>

// Pure, allocation-free menu navigation for small monochrome screens.
// No radio calls, BLE traffic, heap allocations, timers, or blocking I/O.
class EricssonMenuModel {
public:
  explicit EricssonMenuModel(uint8_t itemCount = 0)
      : _itemCount(itemCount), _selected(0) {}

  void setItemCount(uint8_t count) {
    _itemCount = count;
    if (_itemCount == 0 || _selected >= _itemCount) _selected = 0;
  }

  uint8_t count() const { return _itemCount; }
  uint8_t selected() const { return _selected; }

  void up() {
    if (_itemCount == 0) return;
    _selected = (_selected == 0) ? (_itemCount - 1) : (_selected - 1);
  }

  void down() {
    if (_itemCount == 0) return;
    _selected = (_selected + 1 == _itemCount) ? 0 : (_selected + 1);
  }

  uint8_t firstVisible(uint8_t rows) const {
    if (rows == 0 || _itemCount <= rows || _selected < rows) return 0;
    return _selected - rows + 1;
  }

private:
  uint8_t _itemCount;
  uint8_t _selected;
};
