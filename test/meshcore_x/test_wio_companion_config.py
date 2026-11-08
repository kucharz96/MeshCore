"""Static regression checks for official Wio Tracker L1 Companion build variants.

Run: python3 -m unittest discover -s test/meshcore_x -p 'test_*.py'
No radio hardware, PlatformIO dependencies, or third-party Python packages required.
"""
import configparser
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[2]
VARIANT = ROOT / "variants/wio-tracker-l1/platformio.ini"
ROOT_INI = ROOT / "platformio.ini"
TARGETS = ("WioTrackerL1_companion_radio_ble", "WioTrackerL1_companion_radio_usb")


def read_ini(path):
    parser = configparser.ConfigParser(interpolation=None, strict=True)
    parser.optionxform = str
    with path.open(encoding="utf-8") as source:
        parser.read_file(source)
    return parser


def flags(section):
    return set(re.findall(r"(?m)^\s*(-D\s*[A-Za-z_][A-Za-z_0-9]*(?:=[^\s;]+)?)", section.get("build_flags", "")))


class WioCompanionConfigurationTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.variant = read_ini(VARIANT)
        cls.root = read_ini(ROOT_INI)

    def test_targets_exist_and_extend_same_board(self):
        for target in TARGETS:
            with self.subTest(target=target):
                self.assertIn("env:" + target, self.variant)
                self.assertEqual(self.variant["env:" + target]["extends"], "WioTrackerL1")

    def test_shared_sx1262_board_configuration(self):
        common = self.variant["WioTrackerL1"]
        self.assertEqual(common["board"], "seeed-wio-tracker-l1")
        self.assertIn("-D USE_SX1262", flags(common))
        self.assertIn("-D WIO_TRACKER_L1", flags(common))
        self.assertIn("CustomSX1262", common["build_flags"])

    def test_transport_separation(self):
        ble = flags(self.variant["env:" + TARGETS[0]])
        usb = flags(self.variant["env:" + TARGETS[1]])
        self.assertIn("-D ENABLE_USB_INTERFACE", usb)
        self.assertNotIn("-D ENABLE_USB_INTERFACE", ble)
        self.assertIn("-D BLE_PIN_CODE=123456", ble)
        self.assertNotIn("-D BLE_PIN_CODE=123456", usb)

    def test_companion_display_and_storage_flags(self):
        for target in TARGETS:
            with self.subTest(target=target):
                section = self.variant["env:" + target]
                defined = flags(section)
                for required in ("-D DISPLAY_CLASS=SH1106Display", "-D UI_HAS_JOYSTICK=1",
                                 "-D QSPIFLASH=1", "-D UI_NO_HIBERNATE"):
                    self.assertIn(required, defined)
                self.assertIn("+<../examples/companion_radio/*.cpp>", section["build_src_filter"])

    def test_no_transport_specific_rf_override(self):
        # RF parameters remain inherited from the official board/base configuration.
        rf_names = ("LORA_TX_POWER", "LORA_FREQ", "LORA_BW", "LORA_SF",
                    "SX126X_CURRENT_LIMIT", "SX126X_RX_BOOSTED_GAIN")
        for target in TARGETS:
            with self.subTest(target=target):
                defined = flags(self.variant["env:" + target])
                self.assertFalse(any(re.match(r"-D\s*" + name + r"(?:=|$)", flag)
                                     for flag in defined for name in rf_names))

    def test_native_test_target_present(self):
        self.assertEqual(self.root["env:native"]["test_framework"], "googletest")


if __name__ == "__main__":
    unittest.main()
