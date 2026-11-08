# MeshCore X — Ericsson inbox (stage 3)

This is an experimental **stacked draft PR** on top of the Ericsson Contacts stage 2 (PR #5), which itself depends on stage 1 (PR #4). PR #1 is untouched.

## Functionality

- Wio Tracker L1 Pro joystick: **Home → Enter → Wiadomosci → Enter** opens a real message list on the 128×64 monochrome OLED.
- Scroll recent direct and channel messages newest-first with UP/DOWN.
- PRESS/RIGHT opens the selected message; UP/DOWN or PRESS advances/reverses 38-character pages of message text. LEFT/BACK returns to list, then main menu.
- The UI marks channel messages with a `#`; each entry preserves sender name, incoming text, receive timestamp and channel/direct origin.
- The newest 16 incoming messages are retained in RAM until power loss or reboot. The sender name is capped at 32 bytes; text at 111 bytes. Long messages are **truncated in the local UI cache**, never truncated in the official Companion queue.
- The existing automatic incoming-message preview and notification behavior remains in place.

## Crucial protocol and radio invariants

The `MyMesh::queueMessage()` code and its `addToOfflineQueue()` / `getFromOfflineQueue()` operations are **not modified**. The new inbox only takes bounded copies when the official `UITask::onMessageRecv()` or `UITask::onChannelMessageRecv()` callback fires.

Reading a message on OLED does **not** dequeue it or send acknowledgements to the phone. There are no new BLE commands, packet formats, RF settings, LoRa transmissions, filesystem writes, heap allocations per message, or changes to RX scheduling.

The cache is not a permanent message store or a complete inbox; users must still sync with the official MeshCore iOS application to preserve messages beyond the RAM cache window.

## Memory and performance

- At most 16 entries × approximately 152 bytes = roughly 2.5 KiB of static UI memory, plus a small screen object and bounded stack scratch buffers.
- Insert is constant time; no packet processing is blocked by sorting or flash I/O.
- Each list render reads at most three entries; detail view reads only the selected entry.
- Long text is copied with explicit NUL termination. The display uses the existing UTF-8-to-OLED block conversion without changing the radio payload.

## Tests

Native GoogleTest:
- Empty inbox, safe navigation with 0 messages.
- Newest-first order and channel-vs-direct marker.
- Long sender names and message bodies are truncated and terminated.
- Null input safety.
- Ring overflow and last-16 retention.
- Wraparound, viewport bounds and selected-message stability on new arrivals.
- One million mixed arrivals/joystick operations without index overflow.

Build tests: `WioTrackerL1_companion_radio_ble` and `WioTrackerL1_companion_radio_usb`.

**Device validation still required**: joystick directions/softkeys, actual OLED text clipping, UTF-8 messages, Bluetooth reconnect and queue sync on iOS, high message arrival rate, heap usage and overall battery draw. CI cannot prove RX sensitivity or power savings.
