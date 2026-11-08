#pragma once

#if defined(WIO_TRACKER_L1)
#include <helpers/ui/UIScreen.h>

class UITask;

// A compact, non-destructive inbox viewer. All messages remain in the
// official MeshCore Companion phone synchronization queue.
class EricssonInboxScreen : public UIScreen {
public:
  explicit EricssonInboxScreen(UITask* task)
      : _task(task), _details(false), _page(0) {}

  void open();
  int render(DisplayDriver& display) override;
  bool handleInput(char key) override;

private:
  UITask* _task;
  bool _details;
  uint8_t _page;
};
#endif
