# MeshCore X — completion contract and release gates

This file defines **when to stop iterating** instead of creating endless speculative changes. Applies to the existing single development branch `meshcore-x/ericsson-inbox-stage3` and draft PR #6. Its purpose is to produce a stable **release candidate** for Wio Tracker L1 Pro without losing functionality provided by official MeshCore.

## Definition of complete: two separate milestones

### A. Software complete (automatable)

A software-complete candidate must satisfy **all** of these conditions:

- [ ] Baseline is pinned to an official `meshcore-dev/MeshCore` commit. Every subsequent upstream release is reviewed for applicable API, firmware, security and radio changes. No unreviewed functional regression remains.
- [ ] Every existing official Wio L1 build succeeds: Companion BLE, Companion USB, Repeater, Room Server and KISS Modem. Preserve their options and support code.
- [ ] All native tests pass, including deterministic stress/error-injection around queues, rollover, empty/full tables, message retention, CLI framing, joystick/menu navigation and timers.
- [ ] Official Companion request/response/push code numbering and command handling remain compatible. Compare representative device-query, message-send, message-sync, contacts, discovery, channel, stats, CLI, pin setup and radio-param flows with official behavior.
- [ ] Static analysis and build results show no newly introduced critical warnings, memory-safety failures or code-size overflows for Wio L1 Pro.
- [ ] New UI is responsive and independent of radio/transport queues; it does not acknowledge or consume iOS pending messages. Bounded memory usage and no flash writes inside hot UI paths.
- [ ] Proposed features have a documented purpose, a measurable test and a fallback if they might impair RX/TX, BLE interoperability or power use.
- [ ] No known reproducible blocker or high-severity defect is open (P0/P1 = 0). All accepted functional requirements have tests. Any nonblocking limitation is listed.
- [ ] Two consecutive runs on an unchanged release-candidate commit pass the complete automated gate. Any subsequent code change invalidates the gate and requires retesting.

**Software complete is not a claim of on-device reliability.** If software gates pass but hardware testing is unavailable, stop inventing features solely to keep an hourly job busy. Mark the candidate `AWAITING_HARDWARE_VALIDATION` and document the exact tests and equipment needed.

### B. Hardware validated (requires physical Wio devices)

This milestone is necessary before recommending an end-user firmware image:

- [ ] Flash on at least two Wio Tracker L1 Pro boards, verify boot, OLED and all five joystick controls with GPS and buzzer where installed.
- [ ] Verify BLE pairing/re-pairing, iOS Companion compatibility, USB Companion and full message synchronization while using the local UI. Test no-phone and connected-phone scenarios.
- [ ] Perform a documented A/B test against the pinned official MeshCore build using identical board revisions, antennas, positions, legal frequency/power and SF/BW/CR. At least 1000 received packets per tested RF condition (strong, marginal, interference/congestion). The candidate must not decrease delivery probability by more than 1 percentage point versus official in any measured condition; report uncertainty intervals and raw counts. If test statistics do not permit a decision, collect more data.
- [ ] Compare TX confirmation, acknowledgments, retries, stuck-RX recovery and BLE reconnect under controlled fault injection. Any regression in delivery/latency must be investigated rather than averaged away.
- [ ] Measure energy with an external meter in repeatable idle RX, active mesh traffic, BLE-connected, display-on and GPS-on workloads. Candidate must not consume more energy than official in the *same* workload unless the increase is quantified and explicitly accepted for a separately measurable gain. Voltage alone is not a battery-consumption measurement.
- [ ] Complete at least 72 continuous hours of operation without a firmware crash, deadlock, watchdog restart or unrecoverable radio/BLE failure, recording actual uptime and event counts.
- [ ] Confirm all RF settings respect device absolute maximums and local legal emission/airtime requirements; disable/revert any setting whose safety or compliance is unproven.
- [ ] Record test dates, firmware SHAs, raw logs, current traces, RF parameters and deviations for independent reproduction.

### C. Stop and freeze decision

When **A and B** pass, no P0/P1 defects remain, and two consecutive full verification cycles find no new regressions, label the build `RELEASE_CANDIDATE_ACCEPTED`, produce an immutable source SHA, test evidence and changelog, and **freeze feature development**. Move from feature work into upstream/security maintenance.

If performance experiments fail to exceed the current verified baseline, revert or disable them and **stop optimizing that dimension** rather than repeatedly inventing ungrounded changes. If a gate fails, fix the concrete failure before adding other features. If hardware tests are unavailable, stay in `AWAITING_HARDWARE_VALIDATION`.

No engineering process can certify that there will never be another bug or improvement; therefore the acceptable conclusion is **“all agreed acceptance gates passed; no known release blockers”**, not an unsupported “100% bug-free forever”.

## Iteration contract (required for every autonomous run)

Each session must create and close a **finite work unit**:

1. **Scope:** one concrete defect, functionality gap or measurable optimization. Cite the source of the need.
2. **Acceptance:** define a reproducible test and observable pass/fail outcome **before** changing production behavior.
3. **Change:** modify only the existing development branch. Do not create another branch or PR and do not push directly to `main`.
4. **Verification:** native tests + applicable five Wio builds + protocol parity check. If an actual radio measurement is necessary but inaccessible, report the exact remaining gate; do not simulate success.
5. **Decision:** PASS, FAIL/BLOCKED, or REVERT. A failed experiment must not remain enabled as a purported optimization.
6. **Report:** record SHA, test links and results, remaining gates, next bounded task. If no justified tasks remain, issue the milestone verdict and stop feature creation.

Keep PR #1 unchanged. The single rolling implementation is PR #6, whose base is an existing earlier feature branch; no direct change is merged to `main` without explicit review.
