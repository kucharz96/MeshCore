#pragma once

#if defined(WIO_TRACKER_L1)

#include <helpers/ui/UIScreen.h>
#include "EricssonMenuModel.h"

class UITask;

// First-stage Ericsson-inspired menu for the Wio Tracker L1 Pro SH1106 OLED.
// No changes to MeshCore radio, routing or BLE Companion protocol.
class EricssonMenuScreen : public UIScreen {
  UITask* _task;
  EricssonMenuModel _menu;

  void activate();

public:
  explicit EricssonMenuScreen(UITask* task) : _task(task), _menu(8) {}
  int render(DisplayDriver& display) override;
  bool handleInput(char c) override;
};

#endif
