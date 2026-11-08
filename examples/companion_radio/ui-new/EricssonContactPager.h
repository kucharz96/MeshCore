#pragma once

#include <stdint.h>

// Small, deterministic contact-browser cursor. The model has no dependencies
// on Arduino, radio, BLE, filesystem or heap allocations.
class EricssonContactPager {
public:
  explicit EricssonContactPager(uint16_t count = 0)
      : _count(count), _selected(0) {}

  void reset(uint16_t count) {
    _count = count;
    _selected = 0;
  }

  void setCount(uint16_t count) {
    _count = count;
    if (_count == 0 || _selected >= _count) _selected = _count ? _count - 1 : 0;
  }

  uint16_t count() const { return _count; }
  uint16_t selected() const { return _selected; }
  bool empty() const { return _count == 0; }

  void previous() {
    if (_count == 0) return;
    _selected = _selected == 0 ? _count - 1 : _selected - 1;
  }

  void next() {
    if (_count == 0) return;
    _selected = (_selected + 1 == _count) ? 0 : _selected + 1;
  }

  uint16_t firstVisible(uint8_t rows) const {
    if (_count == 0 || rows == 0 || _selected < rows) return 0;
    return _selected - rows + 1;
  }

private:
  uint16_t _count;
  uint16_t _selected;
};
