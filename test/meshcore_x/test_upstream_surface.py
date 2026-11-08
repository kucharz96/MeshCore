"""Negative tests for official MeshCore X source and Wio config retention."""
import unittest

from scripts.check_meshcore_upstream_surface import (
    check_surface,
    compare_wio_config,
    flags,
    retained_paths,
)

OFFICIAL = """
[WioTrackerL1]
extends = nrf52_base
build_flags = ${nrf52_base.build_flags}
  -D WIO_TRACKER_L1
  -D USE_SX1262
  -D SX126X_RX_BOOSTED_GAIN=1
build_src_filter = ${nrf52_base.build_src_filter}
  +<WioTrackerL1Board.cpp>

[env:WioTrackerL1_companion_radio_ble]
extends = WioTrackerL1
build_flags = ${WioTrackerL1.build_flags}
  -D BLE_PIN_CODE=123456
  -D DISPLAY_CLASS=SH1106Display
build_src_filter = ${WioTrackerL1.build_src_filter}
  +<../examples/companion_radio/*.cpp>

[env:WioTrackerL1_companion_radio_usb]
extends = WioTrackerL1
build_flags = ${WioTrackerL1.build_flags}
  -D ENABLE_USB_INTERFACE
"""

PATHS = {
    "src/core.cpp",
    "src/radio.h",
    "examples/companion_radio/main.cpp",
    "variants/wio-tracker-l1/platformio.ini",
    "boards/wio.json",
    "docs/README.md",
}


class UpstreamSurfaceTests(unittest.TestCase):
    def test_unmodified_official_source_passes(self):
        self.assertEqual([], check_surface(PATHS, PATHS, OFFICIAL, OFFICIAL))

    def test_additional_source_and_flags_are_allowed(self):
        expanded = OFFICIAL.replace(
            "  -D USE_SX1262",
            "  -D USE_SX1262\n  -D MESHCORE_X_DIAGNOSTICS",
        )
        self.assertEqual(
            [], check_surface(PATHS, PATHS | {"src/meshcore_x.cpp"}, OFFICIAL, expanded)
        )

    def test_missing_official_source_is_rejected(self):
        result = check_surface(
            PATHS, PATHS - {"src/radio.h"}, OFFICIAL, OFFICIAL
        )
        self.assertTrue(any("src/radio.h" in error for error in result))

    def test_nonproduction_docs_not_subject_to_source_gate(self):
        self.assertEqual(
            set(), retained_paths({"docs/README.md", "notes.txt"})
        )

    def test_removed_board_configuration_is_rejected(self):
        result = compare_wio_config(
            OFFICIAL, OFFICIAL.replace("  -D USE_SX1262\n", "")
        )
        self.assertTrue(any("USE_SX1262" in error for error in result))

    def test_modified_radio_setting_is_rejected(self):
        result = compare_wio_config(
            OFFICIAL,
            OFFICIAL.replace("SX126X_RX_BOOSTED_GAIN=1", "SX126X_RX_BOOSTED_GAIN=0"),
        )
        self.assertTrue(any("SX126X_RX_BOOSTED_GAIN" in error for error in result))

    def test_missing_companion_variant_is_rejected(self):
        changed = OFFICIAL.split("[env:WioTrackerL1_companion_radio_usb]")[0]
        result = compare_wio_config(OFFICIAL, changed)
        self.assertTrue(any("companion_radio_usb" in error for error in result))

    def test_changed_base_inheritance_is_rejected(self):
        changed = OFFICIAL.replace("extends = WioTrackerL1", "extends = nrf52_base")
        result = compare_wio_config(OFFICIAL, changed)
        self.assertTrue(any("extends changed" in error for error in result))

    def test_removed_source_filter_is_rejected(self):
        changed = OFFICIAL.replace(
            "  +<../examples/companion_radio/*.cpp>\n", ""
        )
        result = compare_wio_config(OFFICIAL, changed)
        self.assertTrue(any("source filter removed" in error for error in result))

    def test_flags_preserve_values(self):
        self.assertEqual({"A": "", "B": "9"}, flags("  -D A\n  -D B=9\n"))


if __name__ == "__main__":
    unittest.main()
