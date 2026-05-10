import unittest

from tools.sle_team_python_sim import SimulationConfig, simulate_one_run


class OneVsTwentySimulationTest(unittest.TestCase):
    def test_end_to_end_join_report_failover_recover(self) -> None:
        cfg = SimulationConfig(
            member_count=20,
            direct_connection_cap=8,
            fail_relay_at_tick=6,
            recover_relay_at_tick=10,
            ticks_total=14,
        )
        result = simulate_one_run(cfg)

        self.assertEqual(result.discovered_members, 20)
        self.assertEqual(result.approved_members, 20)
        self.assertGreaterEqual(result.report_success_before_failover, 20 * 2)
        self.assertGreaterEqual(result.report_success_during_failover, (20 - 1) * 2)
        self.assertGreaterEqual(result.report_success_after_recover, 20 * 2)
        self.assertGreaterEqual(result.route_reparent_total, 1)
        self.assertGreaterEqual(result.relay_reselection_total, 1)

    def test_stress_runs_are_stable(self) -> None:
        cfg = SimulationConfig(
            member_count=20,
            direct_connection_cap=8,
            fail_relay_at_tick=5,
            recover_relay_at_tick=9,
            ticks_total=12,
        )
        for _ in range(10):
            result = simulate_one_run(cfg)
            self.assertEqual(result.discovered_members, 20)
            self.assertEqual(result.approved_members, 20)
            self.assertGreater(result.total_report_success, 0)

    def test_packet_loss_and_jitter_take_effect(self) -> None:
        cfg = SimulationConfig(
            member_count=20,
            direct_connection_cap=8,
            fail_relay_at_tick=6,
            recover_relay_at_tick=10,
            ticks_total=14,
            packet_loss_rate=0.25,
            jitter_min_ms=0,
            jitter_max_ms=8,
        )
        result = simulate_one_run(cfg)
        self.assertEqual(result.discovered_members, 20)
        self.assertEqual(result.approved_members, 20)
        self.assertGreater(result.report_dropped + result.report_delayed, 0)

    def test_batch_relay_failover_is_handled(self) -> None:
        cfg = SimulationConfig(
            member_count=20,
            direct_connection_cap=8,
            fail_relay_at_tick=0,  # disable single event by choosing out-of-range
            recover_relay_at_tick=20,
            ticks_total=14,
            batch_fail_relay_count=2,
            batch_fail_relay_ticks=[5, 9],
        )
        result = simulate_one_run(cfg)
        self.assertEqual(result.discovered_members, 20)
        self.assertEqual(result.approved_members, 20)
        self.assertGreaterEqual(result.batch_fail_events, 1)
        self.assertGreaterEqual(result.relay_reselection_total, 1)


if __name__ == "__main__":
    unittest.main()
