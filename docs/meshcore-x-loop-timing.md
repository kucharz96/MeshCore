# MeshCore X — Wio Tracker L1 Pro loop timing (stage 4)

This change extends the **same** branch and draft PR #6. No new feature branch or PR is created. Base protocol, radio registers, BLE framing, TX output, packet scheduling and `main` are unchanged.

## Diagnostic commands

On Wio L1 Pro Companion firmware with official MeshCore CLI support, query:

```text
get x.loop
get x.loop.parts
```

Example **format**, NOT an actual hardware measurement:

```text
> loops=10000 max_us=12450 over10=4 over50=0
> mesh=6300 iface=200 sensor=150 ui=8800 other=320
```

Fields:
- `loops`: number of foreground loop samples since boot (saturates at `UINT32_MAX`)
- `max_us`: maximum **active** loop duration, microseconds
- `over10` and `over50`: loops lasting at least 10 ms or 50 ms, respectively
- `mesh`, `iface`, `sensor`, `ui`, `other`: independent maximum microseconds spent in `the_mesh.loop()`, `interface_manager.loop()`, `sensors.loop()`, `ui_task.loop()`, and `rtc_clock.tick() + board.loop() + watchdog`

Each stage maximum may come from **a different loop iteration**; therefore stage maxima need not sum to the reported overall maximum.

## Implementation

- Only built for `WIO_TRACKER_L1` Companion. All other boards compile their unchanged loop.
- Uses `micros()` timestamps around existing, unmodified loop calls.
- Unsigned 32-bit subtraction is rollover-safe for single intervals shorter than the ~71-minute `micros()` wrap period.
- Snapshot counters have no dynamic allocation, file I/O, flash writes, logging, RF packets or periodic BLE transmissions. Values are returned **only when queried** via the original CLI response frame.
- Deliberately excludes `board.sleep(0)` from active timing: a productive MCU sleep is not interpreted as radio starvation. The metrics therefore do **not** measure scheduler wakeup latency or LoRa IRQ-to-user-callback latency.
- The observer itself has small CPU overhead (several calls to `micros()` per loop). It cannot prove that battery consumption improved; its own overhead should be measured on hardware.

## Related iOS coexistence fix

On Wio L1 Pro only, clearing the **original iOS synchronization queue** now returns the user to Home **only if the legacy message preview is actually visible**. Previously the queue reaching zero forced Home regardless of which screen was being browsed. This change lets the Ericsson inbox and contacts remain open while the official iOS app synchronizes messages, without changing queue contents, ACKs or BLE commands.

## Acceptance and follow-up

1. Run native PlatformIO tests (initial state, independent stage maxima, inclusive thresholds, timer wraparound, reset and one million samples).
2. Build the Wio L1 Pro Companion BLE and USB variants; check that other Wio and non-Wio targets still compile.
3. With a real Wio board and a stable radio peer, log `get x.loop`/parts during idle, high-traffic RX, USB/iOS sync, joystick use, OLED redraw, GPS activity and large contact saves.
4. Record measured max loop latency and event rates per configuration, with the exact commit, firmware, RF settings and whether connected to iOS. Do not conflate active loop latency with RX air-interface quality.
5. When hardware measurements show specific stalls, optimize only those identified functions and compare against the official MeshCore baseline under identical workloads.
6. For battery-life claims, use an external current meter with time-series recording, not voltage alone.

**CI cannot confirm RF sensitivity, Bluetooth stability in the field, OLED appearance, or runtime battery improvements.**
