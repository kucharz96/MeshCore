#!/usr/bin/env python3
"""Fail early when MeshCore X drops official source/build surface.

This protects the Wio Tracker L1 official capabilities against accidental
deletion, independently of the protocol command-number compatibility gate.
It is a *static guard*: successful builds and on-device tests are still needed.
"""
from __future__ import annotations

import argparse
import configparser
import re
import subprocess
import sys
from pathlib import Path

OFFICIAL_BASELINE = "81dbae592ffad40b4212bc0b37e82b38da08f4bc"
WIO_INI = "variants/wio-tracker-l1/platformio.ini"
RETAINED_ROOTS = ("src/", "examples/", "variants/", "boards/", "lib/")
SOURCE_EXTENSIONS = (".c", ".cpp", ".h", ".hpp", ".ino", ".ini", ".json", ".py")
FLAG_PATTERN = re.compile(r"(?m)^\s*-D\s+([A-Za-z_][A-Za-z_0-9]*)(?:=([^\n;]*))?\s*(?:;.*)?$")


def git_text(*args: str) -> str:
    result = subprocess.run(["git", *args], check=False, capture_output=True, text=True)
    if result.returncode:
        raise RuntimeError(f"git {' '.join(args)} failed: {result.stderr.strip()}")
    return result.stdout


def retained_paths(paths: set[str]) -> set[str]:
    return {
        path for path in paths
        if path.startswith(RETAINED_ROOTS) and path.endswith(SOURCE_EXTENSIONS)
    }


def parse_ini(content: str) -> configparser.ConfigParser:
    parser = configparser.ConfigParser(interpolation=None)
    parser.optionxform = str
    parser.read_string(content)
    return parser


def flags(content: str) -> dict[str, str | None]:
    return {
        name: raw.strip() if raw is not None else None
        for name, raw in FLAG_PATTERN.findall(content)
    }


def compare_wio_config(original: str, candidate: str) -> list[str]:
    baseline, current = parse_ini(original), parse_ini(candidate)
    errors: list[str] = []

    for section in baseline.sections():
        if section not in current:
            errors.append(f"removed official Wio platform section: [{section}]")
            continue
        source = baseline[section]
        target = current[section]

        if source.get("extends") != target.get("extends"):
            errors.append(f"[{section}] extends changed or removed")

        old_defs = flags(source.get("build_flags", ""))
        new_defs = flags(target.get("build_flags", ""))
        for name, value in sorted(old_defs.items()):
            if name not in new_defs or new_defs[name] != value:
                errors.append(f"[{section}] original -D {name}={value!r} missing/changed")

        # Includes the official Companion, radio and board source trees.
        old_filters = {
            line.strip() for line in source.get("build_src_filter", "").splitlines()
            if line.strip().startswith(("+<", "-<"))
        }
        new_filters = {
            line.strip() for line in target.get("build_src_filter", "").splitlines()
            if line.strip().startswith(("+<", "-<"))
        }
        for item in sorted(old_filters - new_filters):
            errors.append(f"[{section}] original build source filter removed: {item}")

    return errors


def check_surface(base_paths: set[str], current_paths: set[str],
                  original_ini: str, current_ini: str) -> list[str]:
    errors = [
        f"official source/build file removed: {path}"
        for path in sorted(retained_paths(base_paths) - retained_paths(current_paths))
    ]
    errors.extend(compare_wio_config(original_ini, current_ini))
    return errors


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--baseline", default=OFFICIAL_BASELINE)
    args = parser.parse_args()

    official_files = set(git_text("ls-tree", "-r", "--name-only", args.baseline).splitlines())
    working_files = set(git_text("ls-files", "--cached").splitlines())
    official_ini = git_text("show", f"{args.baseline}:{WIO_INI}")
    current_ini = Path(WIO_INI).read_text(encoding="utf-8")
    errors = check_surface(official_files, working_files, official_ini, current_ini)

    print(f"Official reference: {args.baseline}")
    print(f"Retained source/build paths: {len(retained_paths(official_files))}")
    print(f"Official Wio sections: {len(parse_ini(official_ini).sections())}")
    if errors:
        print("FAIL: removed official MeshCore functionality surface:", file=sys.stderr)
        for error in errors:
            print(f"  - {error}", file=sys.stderr)
        return 1

    print("PASS: original source/build surface and Wio configuration retained")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
