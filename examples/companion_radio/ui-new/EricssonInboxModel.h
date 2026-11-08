#pragma once

#include <stddef.h>
#include <stdint.h>

// Wio L1 Companion-only, RAM-backed read-only history of recent text messages.
// Intentionally independent of MeshCore's outgoing/offline phone-sync queue:
// viewing a message here never acknowledges, removes or mutates phone data.
class EricssonInboxModel {
public:
  static constexpr uint8_t Capacity = 16;
  static constexpr size_t SenderBytes = 33;
  static constexpr size_t TextBytes = 112;

  struct Entry {
    uint32_t receivedAt;
    bool channel;
    char sender[SenderBytes];
    char text[TextBytes];
  };

  EricssonInboxModel() : _next(0), _count(0), _selected(0) {}

  uint8_t count() const { return _count; }
  uint8_t selected() const { return _selected; }
  bool empty() const { return _count == 0; }

  // Copies only a bounded prefix. Does not allocate or touch flash or radio.
  // This is a separate UI cache; the original message remains in MeshCore.
  void add(const char* sender, const char* text, uint32_t receivedAt, bool channel) {
    // The oldest selected entry is about to be overwritten when full.\n    // Move to the incoming message instead of silently showing another entry.\n    const bool selectedWillBeOverwritten = _count == Capacity && _selected == Capacity - 1u;\n    Entry& e = _entries[_next];
    e.receivedAt = receivedAt;
    e.channel = channel;
    copyLimited(e.sender, SenderBytes, sender);
    copyLimited(e.text, TextBytes, text);
    _next = static_cast<uint8_t>((_next + 1u) % Capacity);
    if (_count < Capacity) ++_count;
    // Keep the same selected message when new messages arrive during browsing
    // (unless the circular buffer has overwritten it).
    if (selectedWillBeOverwritten) _selected = 0;\n    else if (_selected < _count - 1u) ++_selected;
  }

  void selectLatest() { _selected = 0; }

  void previous() {
    if (_count == 0) return;
    _selected = _selected == 0 ? _count - 1u : _selected - 1u;
  }

  void next() {
    if (_count == 0) return;
    _selected = (_selected + 1u == _count) ? 0 : _selected + 1u;
  }

  uint8_t firstVisible(uint8_t rows) const {
    if (rows == 0 || _count == 0 || _selected < rows) return 0;
    return _selected - rows + 1u;
  }

  // Offset 0 is the newest message; other offsets go backwards in time.
  const Entry* at(uint8_t newestOffset) const {
    if (newestOffset >= _count) return nullptr;
    const uint8_t slot =
        static_cast<uint8_t>((_next + Capacity - 1u - newestOffset) % Capacity);
    return &_entries[slot];
  }

  const Entry* current() const { return at(_selected); }

private:
  static void copyLimited(char* dest, size_t capacity, const char* source) {
    if (capacity == 0) return;
    size_t i = 0;
    if (source) {
      while (i + 1 < capacity && source[i] != '\0') {
        dest[i] = source[i];
        ++i;
      }
    }
    dest[i] = '\0';
  }

  Entry _entries[Capacity] {};
  uint8_t _next;
  uint8_t _count;
  uint8_t _selected;
};
