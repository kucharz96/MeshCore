# MeshCore X: Wio Tracker L1 Pro radio-health baseline

Status: **experimental diagnostics; no claimed RX/TX, range or battery improvement**.

This change adds read-only, allocation-free counters to the official RadioLib wrapper for **WioTrackerL1 Companion BLE and USB**. Other platforms, Wio repeater and room server builds retain their original behavior.

## Access

On a Companion client that supports the official `CMD_RUN_CLI_COMMAND` (66), issue:

```text
get x.radio
```

Example reply (illustrative, not from hardware):

```text
> rx_arm=0 rx_read=0 tx_start=0 agc=0
```

There is **no new packet type, BLE characteristic, unsolicited debug output or radio configuration**.

## Semantics

| Counter | Increment condition | Does not prove |
| --- | --- | --- |
| `rx_arm` | RadioLib `startReceive()` returns a non-success code in either start/rearm path | Hardware receiver permanently stuck or failed signal reception |
| `rx_read` | RadioLib `readData()` fails after IRQ/payload-length handling | All CRC failures or RF packet loss |
| `tx_start` | RadioLib `startTransmit()` returns an error | Missing remote ACK or successful delivery |
| `agc` | Existing `resetAGC()` reaches `doResetAGC()` after its guard | Successful AGC recovery or a power-cycle |

Counters reset at `RadioLibWrapper::begin()`; they saturate at `UINT32_MAX`.
The ISR is unchanged and does **not** update the counters. No additional storage writes or network transmissions are introduced.

The feature is compiled for the existing Wio L1 BLE/USB Companion variants, without changing TX power, RF registers, preambles, message formats, routing or Bluetooth framing.

## Verification protocol

1. Run `pio test -e native -e native_kiss_modem -vv`.
2. Build `WioTrackerL1_companion_radio_ble` and `WioTrackerL1_companion_radio_usb`.
3. On actual Wio L1 hardware with a connected antenna, query `get x.radio` from the official Companion client, disconnect/reconnect BLE and query again.
4. Inject RX/TX driver failures in a controlled harness (not over-the-air), then check each counter.
5. Perform matched A/B hardware tests against the pinned official MeshCore firmware: packet reception counts, TX/ACK, low-SNR behavior, sustained operation and current draw.
6. Record observed radio settings, firmware commit, antenna, test duration and actual battery measurements.

**CI compilation and a mocked RadioLib test are not a substitute for a physical RF or battery test.**

## Design constraints

- Keep packet receive and transmit flow untouched; telemetry is passive.
- Preserve `getStats` protocol and Companion BLE behavior.
- Keep radio powered in normal continuous RX; energy optimization must not assume a duty-cycled receiver is lossless.
- Only merge after tests pass and review; do not classify an untested UF2 as reliable.
