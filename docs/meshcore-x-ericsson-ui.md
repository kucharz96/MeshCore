# MeshCore X — Ericsson-style Wio Tracker L1 menu (stage 1)

**Draft / experimental.** This is a first integrated, monochrome 128×64 OLED menu for Wio Tracker L1 Pro Companion. It does not claim a feature-complete historical Ericsson user interface, a range gain, or lower measured current draw.

## Hardware interactions

The existing Wio board pin mapping already defines joystick UP (pin 25), DOWN (26), LEFT (27), RIGHT (28), and PRESS (29) plus the dedicated BACK button (pin 13). Previously the Companion UI only checked left/right/press; stage 1 connects the missing two directions.

- From the official Home page, press the joystick to open the Ericsson-like list menu.
- **UP / DOWN**: move the highlight through eight items, wrapping at the ends.
- **PRESS / RIGHT**: activate the selected entry.
- **LEFT / BACK button**: return to the existing official Home screen.
- Display auto-off behavior remains owned by the official UI task and does not affect LoRa RX.

This is a user-interface addition to the standard Companion, **not** an alternative mode of radio operation. The previous sideways Home-screen pages are preserved.

## Implemented menu actions

| Entry | Stage-1 behavior |
| --- | --- |
| Wiadomosci | Show the existing unread-message count |
| Kontakty | Show the existing contact count |
| Radio | Show original RadioLib receive/send packet counters |
| Bluetooth | Toggle existing Companion BLE interface |
| GPS | Toggle the official GPS setting, where available |
| Rozgloszenie | Send a normal official self-advertisement |
| Bateria | Read current battery millivolts |
| Powrot | Return to official Home screen |

The list deliberately uses ASCII labels for the SH1106 character rendering path. Message/contact list browsing and full Ericsson-like nested submenus are **future work**, not yet delivered.

## Compatibility and reliability rules

- Built on pristine `main` official MeshCore; independent of existing PR #1.
- No changes to MeshCore radio, routing, packet framing, encryption, BLE protocol or TX power.
- Menu navigation is allocation-free. The existing UI already instantiates screens at startup; there are no new dynamic allocations per joystick event or render.
- New Wio joystick pins are already configured with INPUT_PULLUP by `WioTrackerL1Board::begin()`; `MomentaryButton::begin()` must **not** be used on those pins with its current pull configuration because that would override the pullups.
- Menu renderer returns a 5-second refresh period, while input triggers immediate refresh and existing UI auto-off logic still applies.

## Verification

- `pio test -e native -e native_kiss_modem -vv`: navigation model properties, including 1,000,000 up/down operations.
- `pio run -e WioTrackerL1_companion_radio_ble`
- `pio run -e WioTrackerL1_companion_radio_usb`
- Hardware test on actual Wio L1: all directions, screen clipping, notifications, GPS and Bluetooth reconnect, 72-hour stability.
- Hardware test comparing radio packet delivery and battery current against unchanged official MeshCore is required before claiming RX/energy improvements.

No on-device tests have been performed as part of this pull request.
