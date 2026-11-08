# MeshCore X — Ericsson Contacts (stage 2)

This feature extends the Ericsson-style UI from PR #4 and is deliberately submitted as a **stacked draft PR**, with base branch `meshcore-x/ericsson-ui-stage1`. PR #1 and `main` are untouched.

## What works

- `Kontakty` opens a real, read-only contacts browser instead of a counter popup.
- Wio joystick **UP/DOWN** navigates an address book of up to 350 entries, including wraparound and a three-row scrolling viewport for the 128×64 OLED.
- **PRESS/RIGHT** shows details: contact name, node type, stored outgoing path length or unknown status, and favourite marker.
- **LEFT/BACK** returns from details to the list and from the list to the Ericsson main menu.
- Empty address books show an explicit message. A contact removed while browsing will be handled as an unavailable entry, not dereferenced.
- Reopening contacts starts from the first entry.

## Data and protocol

Contacts are read through official `BaseChatMesh::getContactByIdx()`. The first `MAX_ANON_CONTACTS` slots are reserved for temporary connections, so the list indexes only persisted user contacts; `getNumContacts()` supplies the count.

To avoid starving time-critical radio processing, the renderer fetches **at most three records per list draw, one record for details**. There is no sorting, scanning of the full contact table, flash write, new BLE command, background task, advertising, or network transmission. The menu cursor uses no heap.

The details display names a path length in **bytes**, not hops. The project does not infer true link quality from this value.

## Out of scope

Contact editing/deletion, message composition, message inbox browser, live RF power control, actual battery runtime improvements and physical range improvement. Those require separate staged implementations and real-device tests.

## Validation

- Native unit tests for empty/350-contact cases, wraparound, shrink/deletion, zero visible rows, reopening/reset, full 16-bit count, and one million cursor operations.
- Build Wio Tracker L1 Pro Companion BLE and USB in GitHub Actions.
- Manual hardware check: SH1106 clipping, readability, joystick, detail navigation, phone BLE reconnect, contact creation/removal while browsing, zero contacts and hundreds of contacts.
- Confirm actual RF RX/TX and battery use in matched A/B hardware tests before making performance claims.

**Compilation and native tests do not prove that the OLED looks correct on real hardware or that battery/radio parameters are improved.**
