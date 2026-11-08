"""Protocol parity checker must recognize the official if/else CLI dispatcher."""
import unittest

from scripts.check_meshcore_protocol_parity import (
    dispatched_commands,
    firmware_version,
    protocol_codes,
)


class ProtocolParityTests(unittest.TestCase):
    def test_official_if_else_dispatch_is_detected(self):
        source = """
        if (cmd_frame[0] == CMD_DEVICE_QUERY && len >= 2) { }
        else if (cmd_frame[0] == CMD_APP_START) { }
        else if (cmd_frame[0] == CMD_RUN_CLI_COMMAND && len >= 3) { }
        """
        self.assertEqual(
            {"CMD_DEVICE_QUERY", "CMD_APP_START", "CMD_RUN_CLI_COMMAND"},
            dispatched_commands(source),
        )

    def test_old_switch_dispatch_still_supported(self):
        self.assertEqual(
            {"CMD_GET_STATS"},
            dispatched_commands("switch (cmd) { case CMD_GET_STATS: break; }"),
        )

    def test_missing_dispatch_is_observable(self):
        source = "if (cmd_frame[0] == CMD_DEVICE_QUERY) { }"
        original = dispatched_commands(source)
        mutated = dispatched_commands("if (cmd_frame[0] == CMD_SEND_TXT_MSG) { }")
        self.assertIn("CMD_DEVICE_QUERY", original - mutated)

    def test_command_references_in_comments_are_not_dispatch(self):
        source = "// CMD_DEVICE_QUERY and CMD_APP_START are supported"
        self.assertEqual(set(), dispatched_commands(source))

    def test_parser_handles_spaced_array_index(self):
        source = "if (cmd_frame [ 0 ] == CMD_SEND_TXT_MSG) {}"
        self.assertEqual({"CMD_SEND_TXT_MSG"}, dispatched_commands(source))

    def test_code_numbers_match_decimals_and_hex(self):
        defs = "#define CMD_APP_START 1\n#define PUSH_CODE_MSG_WAITING 0x83\n"
        self.assertEqual(
            {"CMD_APP_START": 1, "PUSH_CODE_MSG_WAITING": 131},
            protocol_codes(defs),
        )

    def test_protocol_version_detection(self):
        self.assertEqual(14, firmware_version("#define FIRMWARE_VER_CODE 14"))


if __name__ == "__main__":
    unittest.main()
