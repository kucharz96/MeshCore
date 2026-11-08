#pragma once

#if defined(WIO_TRACKER_L1)

#include <helpers/ui/UIScreen.h>
#include "EricssonContactPager.h"

class UITask;

// Read-only contacts browser for the Wio Tracker L1 Pro Companion.
// Contact records are read from official MeshCore in small, bounded batches.
class EricssonContactsScreen : public UIScreen {
public:
  explicit EricssonContactsScreen(UITask* task) : _task(task), _pager(), _details(false) {}
  void open();
  int render(DisplayDriver& display) override;
  bool handleInput(char c) override;

private:
  UITask* _task;
  EricssonContactPager _pager;
  bool _details;
};

#endif // WIO_TRACKER_L1
