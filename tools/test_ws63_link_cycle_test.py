import unittest

from tools import ws63_link_cycle_test as lc


class FakeSerial:
    def __init__(self, data_chunks=None):
        self.data_chunks = list(data_chunks or [])
        self.writes = []
        self.closed = False

    def write(self, data):
        self.writes.append(data)
        return len(data)

    def flush(self):
        return None

    def read(self, _n):
        if self.data_chunks:
            return self.data_chunks.pop(0)
        return b""

    def reset_input_buffer(self):
        return None

    def reset_output_buffer(self):
        return None

    def close(self):
        self.closed = True


class LinkCycleUnitTest(unittest.TestCase):
    def test_query_member_online_parses_latest_state(self):
        leader = lc.Peer(name="leader", port="/dev/null", baudrate=115200, ser=FakeSerial())
        member = lc.Peer(name="member", port="/dev/null", baudrate=115200, ser=FakeSerial())

        leader.ser.data_chunks = [
            b"member=2 role=2 online=1 battery=90\n",
            b"member=2 role=2 online=0 battery=89\n",
        ]
        state = lc._query_member_online(leader, [leader, member], member_id=2, query_window_s=0.1)
        self.assertEqual(state, 0)

    def test_wait_regex_matches_target_peer_only(self):
        leader = lc.Peer(name="leader", port="/dev/null", baudrate=115200, ser=FakeSerial([b"pairing start ret=0\n"]))
        member = lc.Peer(name="member", port="/dev/null", baudrate=115200, ser=FakeSerial([b"noise\n"]))
        lc._wait_regex(leader, [leader, member], r"pairing start ret=0", timeout_s=0.2, note="pairing")

    def test_default_channel_matches_firmware_default(self):
        parser = lc.build_parser()

        args = parser.parse_args(["--leader-port", "/dev/tty.fake0", "--member-port", "/dev/tty.fake1"])

        self.assertEqual(args.channel, 17)
        self.assertEqual(args.leader_id, -1)

    def test_route_id_from_suffix_handles_zero_and_broadcast(self):
        self.assertEqual(lc._route_id_from_suffix(0xC700), ((0xC7 % 254) + 1))
        self.assertEqual(lc._route_id_from_suffix(0x12FF), ((0x12 % 254) + 1))
        self.assertEqual(lc._route_id_from_suffix(0x01A2), 0xA2)

    def test_extract_suffix_from_label_and_mac(self):
        self.assertEqual(lc._extract_suffix("label=LC7E9"), 0xC7E9)
        self.assertEqual(
            lc._extract_suffix("mac=AA:BB:CC:DD:C7:E9"),
            0xC7E9,
        )
        self.assertIsNone(lc._extract_suffix("no suffix here"))


if __name__ == "__main__":
    unittest.main()
