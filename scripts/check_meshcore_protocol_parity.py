#!/usr/bin/env python3
"""Protect the official MeshCore Companion wire protocol while developing MeshCore X.

Compares all pre-existing command, reply, push, statistics, and error codes
against the pinned official MeshCore source revision. New identifiers may be
added, but changing or removing official ones fails the PR check.

This is a compatibility guard, NOT proof of full behavior or BLE interoperability.
"""
from __future__ import annotations

import argparse
import re
import subprocess
import sys
from pathlib import Path

OFFICIAL_BASELINE = "81dbae592ffad40b4212bc0b37e82b38da08f4bc"
SOURCE_PATH = "examples/companion_radio/MyMesh.cpp"
HEADER_PATH = "examples/companion_radio/MyMesh.h"
CODE_PREFIXES = ("CMD_", "RESP_CODE_", "PUSH_CODE_", "STATS_TYPE_", "ERR_CODE_")
DEFINE_PATTERN = re.compile(r"^\s*#define\s+([A-Z][A-Z_0-9]+)\s+(0x[0-9a-fA-F]+|[0-9]+)\b", re.MULTILINE)
CASE_PATTERN = re.compile(r"\\bcase\\s+(CMD_[A-Z0-9_]+)\\s*:")
# The official Companion dispatches through if / else if against cmd_frame[0],
# not a switch. The previous case-only matcher silently matched ZERO commands.
FRAME_BRANCH_PATTERN = re.compile(
    r"\\bcmd_frame\\s*\\[\\s*0\\s*\\]\\s*==\\s*(CMD_[A-Z0-9_]+)\\b"
)
FIRMWARE_PATTERN = re.compile(r"^\s*#define\s+FIRMWARE_VER_CODE\s+(\d+)\b", re.MULTILINE)


def load_official(revision: str, path: str) -> str:
    result = subprocess.run(
        ["git", "show", f"{revision}:{path}"],
        capture_output=True, text=True, check=False
    )
    if result.returncode:
        raise RuntimeError(
            f"Cannot load official MeshCore baseline {revision}:{path}: "
            f"{result.stderr.strip()} (checkout must fetch full git history)"
        )
    return result.stdout


def protocol_codes(source: str) -> dict[str, int]:
    return {
        name: int(raw, 0)
        for name, raw in DEFINE_PATTERN.findall(source)
        if name.startswith(CODE_PREFIXES)
    }


def dispatched_commands(source: str) -> set[str]:
    return set(CASE_PATTERN.findall(source)) | set(FRAME_BRANCH_PATTERN.findall(source))


def firmware_version(source: str) -> int:
    match = FIRMWARE_PATTERN.search(source)
    if not match:
        raise ValueError("Missing FIRMWARE_VER_CODE in companion header")
    return int(match.group(1))


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--baseline", default=OFFICIAL_BASELINE)
    args = parser.parse_args()

    official = load_official(args.baseline, SOURCE_PATH)
    candidate = Path(SOURCE_PATH).read_text(encoding="utf-8")
    official_defs = protocol_codes(official)
    candidate_defs = protocol_codes(candidate)

    errors = []
    for name, original in sorted(official_defs.items()):
        actual = candidate_defs.get(name)
        if actual != original:
            errors.append(f"{name}: official={original}, candidate={actual!r}")

    original_cases = dispatched_commands(official)
    current_cases = dispatched_commands(candidate)
    if not original_cases:
        errors.append("baseline parser detected zero official command dispatch paths")
    for name in sorted(original_cases - current_cases):
        errors.append(f"removed official command dispatch: {name}")

    official_v = firmware_version(load_official(args.baseline, HEADER_PATH))
    current_v = firmware_version(Path(HEADER_PATH).read_text(encoding="utf-8"))
    if current_v < official_v:
        errors.append(
            f"FIRMWARE_VER_CODE regressed from {official_v} to {current_v}"
        )

    print(
        f"Official MeshCore baseline: {args.baseline}\n"
        f"Checked {len(official_defs)} protocol code values and "
        f"{len(original_cases)} command dispatch cases; "
        f"firmware protocol version {official_v} -> {current_v}"
    )
    if errors:
        print("FAIL: official Companion compatibility regressions:", file=sys.stderr)
        for error in errors:
            print(f"  - {error}", file=sys.stderr)
        return 1

    print("PASS: official Companion protocol codes and command dispatch preserved")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
