import re
import math
import unittest
from pathlib import Path

from automation.ws63.tools import ws63_four_board_relay_test as fb


class FakeSerial:
    def __init__(self):
        self.writes = []

    def write(self, data):
        self.writes.append(data)
        return len(data)

    def flush(self):
        return None


class FourBoardRelayUnitTest(unittest.TestCase):
    REPO_ROOT = Path(__file__).resolve().parents[3]

    @staticmethod
    def _extract_c_function(source: str, signature: str) -> str:
        search_from = 0
        while True:
            start = source.index(signature, search_from)
            brace = source.find("{", start)
            semicolon = source.find(";", start)
            if brace >= 0 and (semicolon < 0 or brace < semicolon):
                break
            search_from = semicolon + 1 if semicolon >= 0 else start + len(signature)
        brace = source.index("{", start)
        depth = 0
        for index in range(brace, len(source)):
            if source[index] == "{":
                depth += 1
            elif source[index] == "}":
                depth -= 1
                if depth == 0:
                    return source[start : index + 1]
        raise AssertionError(f"function not closed: {signature}")

    @staticmethod
    def _extract_define_int(source: str, name: str) -> int:
        match = re.search(rf"^#define\s+{re.escape(name)}\s+((?:0x)?[0-9A-Fa-f]+)U?\b", source, re.MULTILINE)
        if not match:
            raise AssertionError(f"define not found: {name}")
        return int(match.group(1), 0)

    def test_wait_leader_sees_member_accepts_already_online_record(self):
        leader = fb.lc.Peer(name="leader", port="COML", baudrate=115200, ser=FakeSerial())
        calls = []
        original_query = fb._query_records_once
        original_send = fb._send_and_collect

        def fake_query_records_once(_leader, _peers, window_s=1.0):
            calls.append(("members", window_s))
            return {241: {"online": 1, "relay": 0, "tier": 0, "max_down": 0, "last_seen": 10}}

        def fail_send_and_collect(*_args, **_kwargs):
            raise AssertionError("already-online member should not require pairing pending")

        try:
            fb._query_records_once = fake_query_records_once
            fb._send_and_collect = fail_send_and_collect

            fb._wait_leader_sees_member(
                leader,
                [leader],
                member_id=241,
                leader_id=154,
                timeout_s=1.0,
                poll_s=0.2,
            )
        finally:
            fb._query_records_once = original_query
            fb._send_and_collect = original_send

        self.assertEqual(calls, [("members", 0.2)])
        self.assertEqual(leader.ser.writes, [])

    def test_firmware_has_relay_failover_recovery_guards(self):
        app_source = (self.REPO_ROOT / "xc/ws63_team_network/src/ws63_team_network_app.c").read_text(encoding="utf-8")
        node_source = (self.REPO_ROOT / "src/sle_team_node.c").read_text(encoding="utf-8")
        node_header = (self.REPO_ROOT / "include/sle_team_node.h").read_text(encoding="utf-8")
        display_source = (self.REPO_ROOT / "xc/ws63_team_network/src/ws63_st7789_display.c").read_text(
            encoding="utf-8"
        )
        ws2812_source = (self.REPO_ROOT / "xc/ws63_team_network/src/ws63_ws2812.c").read_text(encoding="utf-8")

        self.assertIn('#define SLE_TEAM_FW_VERSION "v4.4.128"', app_source)
        self.assertIn('#define SLE_TEAM_HW_CONSTRAINTS "v3.2 schematic pinmap, muted buzzer"', app_source)
        self.assertIn("#define CONFIG_SLE_TEAM_WS2812_ENABLE 1", app_source)
        self.assertIn("#define CONFIG_SLE_TEAM_WS2812_PIN 0", app_source)
        self.assertIn("#define CONFIG_SLE_TEAM_BUZZER_ENABLE 1", app_source)
        self.assertIn("#define CONFIG_SLE_TEAM_BUZZER_PIN 14", app_source)
        self.assertIn("#define CONFIG_SLE_TEAM_BUZZER_MUTED 1", app_source)
        self.assertIn("#define CONFIG_SLE_TEAM_GPS_ENABLE 0", app_source)
        self.assertIn("#define CONFIG_SLE_TEAM_GPS_UART_TXD_PIN 17", app_source)
        self.assertIn("#define CONFIG_SLE_TEAM_GPS_UART_RXD_PIN 18", app_source)
        self.assertIn("gps configured=%u present=0 ready=%u", app_source)
        self.assertIn("module=none", app_source)
        self.assertNotIn("module=L80RE-M37", app_source)
        self.assertIn("#define CONFIG_SLE_TEAM_ADC_ENABLE 1", app_source)
        self.assertIn("#define CONFIG_SLE_TEAM_ADC_CTRL_PIN 5", app_source)
        self.assertIn("#define CONFIG_SLE_TEAM_ADC_VBAT_PIN 12", app_source)
        self.assertIn("#define CONFIG_SLE_TEAM_ADC_VBAT_CHANNEL 5", app_source)
        self.assertIn("#define CONFIG_SLE_TEAM_ADC_SAMPLE_SETTLE_MS 50", app_source)
        self.assertIn("#define CONFIG_SLE_TEAM_ADC_SAMPLE_INTERVAL_S 30", app_source)
        self.assertIn("#define SLE_TEAM_ADC_DIVIDER_TOP_KOHM 390U", app_source)
        self.assertIn("#define SLE_TEAM_ADC_DIVIDER_BOTTOM_KOHM 100U", app_source)
        self.assertIn("team_gpio_config_output_level", app_source)
        self.assertIn("#define SLE_TEAM_BATTERY_EMPTY_MV 3300U", app_source)
        self.assertIn("#define SLE_TEAM_BATTERY_FULL_MV 4200U", app_source)
        self.assertIn("uapi_adc_init(ADC_CLOCK_500KHZ)", app_source)
        self.assertIn("adc_port_read(g_team_rt.adc_vbat_channel, &adc_mv)", app_source)
        self.assertIn("team_battery_vbat_mv_from_adc_mv", app_source)
        self.assertIn("team_battery_percent_from_vbat_mv", app_source)
        self.assertIn("team_battery_sample_tick(0U);", app_source)
        self.assertIn("ops.battery_percent = team_battery_percent_cb;", app_source)
        self.assertIn("bat commands: status|sample", app_source)
        self.assertNotIn("sample=0", app_source)
        self.assertIn("team_cli_match2", app_source)
        self.assertIn(
            "return (uint8_t)(strcmp(line, first) == 0 || strcmp(line, second) == 0);",
            app_source,
        )
        self.assertIn(
            "team_http_query_number(path, key, (int64_t)min_value, (int64_t)max_value, 255UL, 0UL, &value) != 0",
            app_source,
        )
        self.assertIn("typedef uint8_t (*sle_team_battery_percent_fn)(void *user_ctx);", node_header)
        self.assertIn("sle_team_battery_percent_fn battery_percent;", node_header)
        self.assertIn("hb.battery_percent = sle_team_battery_percent(node);", node_source)
        self.assertIn("hello.battery_percent = sle_team_battery_percent(node);", node_source)
        self.assertIn("#define CONFIG_SLE_TEAM_ST7789_SCLK_PIN 7", app_source)
        self.assertIn("#define CONFIG_SLE_TEAM_ST7789_MOSI_PIN 9", app_source)
        self.assertIn("#define CONFIG_SLE_TEAM_ST7789_CS_PIN 8", app_source)
        self.assertIn("#define CONFIG_SLE_TEAM_ST7789_CS_ALWAYS_LOW 1", app_source)
        self.assertIn("#define CONFIG_SLE_TEAM_ST7789_DC_PIN 13", app_source)
        self.assertIn("#define CONFIG_SLE_TEAM_ST7789_RESET_PIN 10", app_source)
        self.assertIn("CONFIG_SLE_TEAM_ST7789_CS_ALWAYS_LOW", display_source)
        self.assertIn("ST7789_CS_LOW_SETTLE_MS", display_source)
        self.assertIn("st7789 pins primed", display_source)
        self.assertIn("team_display_init_log", app_source)
        self.assertIn("phase=%s", app_source)
        self.assertIn("panel gnd still needs real board ground", app_source)
        self.assertIn("team_gps_init();", app_source)
        self.assertIn("team_adc_init();", app_source)
        self.assertIn("team_hardware_report_print();", app_source)
        self.assertIn("[hw] init summary fw=%s", app_source)
        self.assertIn("CONFIG_SLE_TEAM_BUZZER_MUTED || !CONFIG_SLE_TEAM_BUZZER_AUTO_TOGGLE", app_source)
        self.assertIn("TEAM_RGB_STATE_LEADER", app_source)
        self.assertIn("TEAM_RGB_STATE_MEMBER", app_source)
        self.assertIn("TEAM_RGB_STATE_ERROR", app_source)
        self.assertIn("#define SLE_TEAM_WS2812_TEST_R 32U", app_source)
        self.assertIn("#define SLE_TEAM_WS2812_TEST_STEP_MS 120U", app_source)
        self.assertIn("#define SLE_TEAM_WS2812_IDLE_R 8U", app_source)
        self.assertIn("#define SLE_TEAM_WS2812_IDLE_G 8U", app_source)
        self.assertIn("#define SLE_TEAM_WS2812_IDLE_B 8U", app_source)
        self.assertIn("#define SLE_TEAM_WS2812_LEADER_G 8U", app_source)
        self.assertIn("#define SLE_TEAM_WS2812_MEMBER_G 0U", app_source)
        self.assertIn("#define SLE_TEAM_WS2812_MEMBER_B 8U", app_source)
        self.assertIn("#define SLE_TEAM_WS2812_ERROR_R 8U", app_source)
        self.assertIn("team_ws2812_cli_set_rgb(line[4] == 'r' ? 8U : (line[4] == 'w' ? 5U : 0U)", app_source)
        self.assertIn("#define SLE_TEAM_WS2812_BREATHE_MIN_SCALE 0U", app_source)
        self.assertIn("#define SLE_TEAM_WS2812_BREATHE_PERIOD_MS 1600U", app_source)
        self.assertIn("#define SLE_TEAM_WS2812_FLASH_PULSES 4U", app_source)
        self.assertIn("timing=cycle-counter", app_source)
        self.assertIn("team_ws2812_test_pattern();", app_source)
        self.assertIn("rdcycle %0", ws2812_source)
        self.assertIn("WS63_WS2812_SLOT_CYCLES", ws2812_source)
        self.assertIn("ws63_ws2812_wait_until_cycle", ws2812_source)
        self.assertNotIn("uapi_tcxo_get_count", ws2812_source)
        self.assertIn(
            "((on != 0U) ^ (g_team_rt.led_active_low != 0U)) ? GPIO_LEVEL_HIGH : GPIO_LEVEL_LOW",
            app_source,
        )
        rgb_base = self._extract_c_function(app_source, "static team_rgb_state_t team_ws2812_base_state")
        self.assertIn("g_team_rt.role_configured == 0U", rgb_base)
        self.assertIn("return TEAM_RGB_STATE_IDLE;", rgb_base)
        self.assertIn("g_team_node.cfg.role == SLE_TEAM_ROLE_LEADER", rgb_base)
        self.assertIn("return TEAM_RGB_STATE_LEADER;", rgb_base)
        self.assertIn("g_team_node.joined != 0U", rgb_base)
        self.assertIn("return TEAM_RGB_STATE_MEMBER;", rgb_base)
        self.assertIn("return TEAM_RGB_STATE_ERROR;", rgb_base)
        rgb_breathe = self._extract_c_function(app_source, "static uint8_t team_rgb_state_is_breathing")
        self.assertIn("state == TEAM_RGB_STATE_IDLE || state == TEAM_RGB_STATE_LEADER", rgb_breathe)
        self.assertIn("state == TEAM_RGB_STATE_MEMBER || state == TEAM_RGB_STATE_ERROR", rgb_breathe)
        rgb_event = self._extract_c_function(app_source, "static void team_ws2812_show_display_event")
        self.assertIn("team_ws2812_start_flash(TEAM_RGB_STATE_ERROR)", rgb_event)
        self.assertIn("team_ws2812_start_flash(TEAM_RGB_STATE_LEADER)", rgb_event)
        self.assertNotIn("team_ws2812_set_state(TEAM_RGB_STATE_TX);", app_source)
        self.assertNotIn("team_ws2812_set_state(TEAM_RGB_STATE_RX);", app_source)
        self.assertNotIn("team_ws2812_set_state(TEAM_RGB_STATE_SEEK);", app_source)
        self.assertIn("member timeout deferred after relay loss", node_source)
        self.assertIn("team_leader_should_seek_member", app_source)
        self.assertIn("team_leader_enforce_direct_capacity", app_source)
        self.assertIn("direct cap migrate member", app_source)
        self.assertIn("direct cap prune confirmed", app_source)
        self.assertIn("direct cap prune disconnect", app_source)
        self.assertIn("direct cap prune deferred", app_source)
        self.assertIn("team_member_relay_can_accept_child", app_source)
        self.assertIn("Route updates are leader policy hints", node_source)
        self.assertIn("route update requests parent reselect", node_source)
        self.assertNotIn("node->upstream_parent_id = route_update.next_hop_id", node_source)
        self.assertIn(
            "if (g_team_node.cfg.pairing_enabled != 0U) {\n        return 1U;\n    }",
            app_source,
        )
        self.assertIn("team_leader_find_member_slot(candidate_id) != NULL", app_source)
        self.assertIn("sle_team_node_is_member_allowed(&g_team_node, candidate_id)", app_source)
        self.assertIn("relay config notify pending", app_source)
        self.assertIn("route_id_provisional", app_source)
        self.assertIn("team_conn_track_route_id_is_trusted", app_source)
        self.assertIn("team_conn_track_route_id_is_valid", app_source)
        self.assertIn("Legacy fallback: address-derived route ids are only provisional.", app_source)
        self.assertIn("team_pending_conn_note(&seek_result_data->addr, candidate_id, candidate_provisional);", app_source)
        self.assertIn("team_leader_relay_target_with_failover", app_source)
        self.assertIn("relay failover holding relay target", app_source)
        self.assertIn("team_leader_known_member_count", app_source)
        self.assertIn("static uint8_t team_leader_relay_budget(void)", app_source)
        self.assertIn("SLE_TEAM_RELAY_MGMT_RAM_BUDGET_BYTES", app_source)
        self.assertIn("SLE_TEAM_RELAY_MGMT_EST_BYTES_PER_RELAY", app_source)
        self.assertIn("demand_count = known_count > online_count ? known_count : online_count;", app_source)
        self.assertIn(
            "relay_target = team_leader_relay_target_with_failover(demand_count, relay_count, now_s);",
            app_source,
        )
        self.assertIn("team_leader_failover_recovery_next_hop_is_valid", app_source)
        recovery_valid = self._extract_c_function(app_source, "static uint8_t team_leader_failover_recovery_next_hop_is_valid")
        self.assertIn("next_hop_id == member_id || next_hop_id == g_team_node.cfg.leader_id", recovery_valid)
        self.assertIn("route hint skip member=%u parent=%u reason=no-route", app_source)
        self.assertIn("team_leader_failover_should_seek_member", app_source)
        self.assertIn("member_id == g_team_rt.relay_failover_lost_id", app_source)
        self.assertIn("direct cap defer reason=failover", app_source)
        self.assertIn("watched = (uint8_t)(watched + team_leader_failover_watch_member(lost_relay_id));", app_source)
        self.assertIn("next_hop_id == member_id", app_source)
        self.assertIn("relay failover member=%u route pending next_hop=%u", app_source)
        self.assertIn("team_leader_route_physical_parent_matches", app_source)
        self.assertIn("reason=physical-parent", app_source)
        self.assertIn("reason=failover-parent-not-ready", app_source)
        self.assertIn("node->joined = 0U;", node_source)
        self.assertIn("node->state = SLE_TEAM_NET_DISCOVERING;", node_source)
        self.assertIn("node->last_hello_s = 0U;", node_source)
        self.assertIn("config send failed on hello; liveness preserved", node_source)
        self.assertIn("hello ack send failed; liveness preserved", node_source)
        self.assertIn("SLE_TEAM_MAIN_LOOP_SLEEP_MS", app_source)
        self.assertIn("osal_msleep(SLE_TEAM_MAIN_LOOP_SLEEP_MS);", app_source)
        self.assertIn("TeamDisplayTask", app_source)
        self.assertIn("SLE_TEAM_DISPLAY_TASK_INTERVAL_MS", app_source)
        self.assertIn("#define SLE_TEAM_DISPLAY_TASK_STACK_SIZE 0x1800", app_source)
        self.assertIn("team_display_start();", app_source)
        self.assertIn("display_status_online_count", app_source)
        self.assertIn("display_status_offline_count", app_source)
        self.assertIn("display_status_event_count", app_source)
        self.assertIn("ST7789_LVGL_HANDLER_MIN_INTERVAL_MS", display_source)
        self.assertIn("delta_ms < ST7789_LVGL_HANDLER_MIN_INTERVAL_MS", display_source)
        self.assertIn("team_member_has_reselect_target", app_source)
        self.assertIn("RESELECT_PARENT", app_source)
        self.assertIn("hello ack deferred until reselect parent", node_source)
        self.assertIn("config deferred until reselect parent", node_source)
        self.assertIn("SLE_TEAM_RELAY_CHILD_RESCAN_INTERVAL_S", app_source)
        self.assertIn("SLE_TEAM_PARENT_RESELECT_DISCONNECT_DELAY_S", app_source)
        self.assertIn("team_member_parent_reselect_disconnect_tick", app_source)
        self.assertIn("parent reselect drop old leader conn", app_source)
        self.assertIn("parent reselect drop upstream", app_source)
        self.assertIn("sle_uart_server_adv_restart()", app_source)
        self.assertIn("Leader-bound relayed packets always go upstream before bucket tier routing.", app_source)
        self.assertIn("} else if (dst_id == g_team_node.cfg.leader_id) {\n        /* Leader-bound relayed packets always go upstream before bucket tier routing. */\n        send_upstream = 1U;\n    } else if (src_bucket < self_bucket)", app_source)
        self.assertIn("sle_team_relay_may_bridge_packet", node_source)
        self.assertIn("team_route_should_bridge_relay_control", app_source)
        self.assertIn("relay control bridge src=%u dst=%u type=%u", app_source)
        self.assertIn("node->cfg.relay_allowed == 0U", node_source)
        self.assertIn("node->joined == 0U ||", node_source)
        self.assertIn("team_route_control_packet_can_bridge(app_packet->app_msg_type)", app_source)
        self.assertIn("team_route_next_hop_is_direct_peer", app_source)
        self.assertIn("Direct next-hop ids use the recorded physical conn_id", app_source)
        self.assertIn("next_hop_id == g_team_node.cfg.self_id", app_source)
        self.assertIn("team_route_next_hop_is_direct_peer(member_id, route->next_hop_id) == 0U", app_source)
        self.assertIn("team_route_metrics_mark_dirty", app_source)
        self.assertIn("team_leader_reconcile_online_routes", app_source)
        self.assertIn("route reconcile member=%u parent=%u conn=%u", app_source)
        self.assertIn("route note member=%u conn=%u dir=%u next=%u", app_source)
        self.assertIn("team_leader_reconcile_online_routes(now_s);", app_source)
        self.assertIn("app_packet.src_id is the logical origin", app_source)
        self.assertIn("physical first hop", app_source)
        self.assertIn("track->route_id_provisional != 0U && track->route_id == app_packet.src_id", app_source)
        self.assertIn("team_leader_pending_member_count", app_source)
        self.assertIn("pending_count = team_leader_pending_member_count();", app_source)
        self.assertIn("known_count", app_source)
        self.assertIn("demand_count", app_source)
        self.assertIn("relay rebalance demand", app_source)
        self.assertIn("relay rejected unicast packet", node_source)
        self.assertNotIn("(const sle_team_route_update_body_t *)app_packet->body", app_source)
        self.assertNotIn("(const sle_team_route_update_body_t *)app_packet.body", app_source)

    def test_cfg_clear_resets_saved_role_and_allowlist(self):
        app_source = (self.REPO_ROOT / "xc/ws63_team_network/src/ws63_team_network_app.c").read_text(encoding="utf-8")
        clear_allowed = self._extract_c_function(app_source, "static int team_nv_allowed_clear")
        clear_all = self._extract_c_function(app_source, "static int team_cfg_clear_all_saved")
        cli_handle = self._extract_c_function(app_source, "static int team_serial_cfg_cli_handle")
        http_handle = self._extract_c_function(app_source, "static void team_http_handle_client")

        self.assertIn("SLE_TEAM_NV_KEY_ALLOWED_MEMBERS", clear_allowed)
        self.assertIn("team_nv_config_clear()", clear_all)
        self.assertIn("team_nv_allowed_clear()", clear_all)
        self.assertIn("sle_team_node_allow_all_members(&g_team_node)", clear_all)
        self.assertIn("team_cfg_clear_all_saved()", cli_handle)
        self.assertIn("team_serial_cfg_cli_done()", cli_handle)
        self.assertIn("team_cfg_apply_role", app_source)
        self.assertIn("g_team_node.cfg.role == SLE_TEAM_ROLE_MEMBER && g_team_node.cfg.leader_id == leader_id", app_source)
        self.assertIn("team_serial_cfg_cli_save_leader", app_source)
        self.assertIn("scope=config+allowlist", cli_handle)
        self.assertIn("team_cfg_clear_all_saved()", http_handle)

    def test_build_scripts_archive_firmware_outputs_without_overwriting(self):
        remote_build = (self.REPO_ROOT / "automation/ws63/tools/ws63_remote_build_v4.py").read_text(encoding="utf-8")
        local_wsl = (self.REPO_ROOT / "scripts/build/ws63_build_v4_local_wsl.sh").read_text(encoding="utf-8")
        ubuntu = (self.REPO_ROOT / "scripts/build/ws63_build_v4_ubuntu.sh").read_text(encoding="utf-8")

        self.assertIn("def versioned_output_path", remote_build)
        self.assertIn("def reserve_unique_path", remote_build)
        self.assertIn("archive_output = reserve_unique_path(versioned_output_path(local_output, VERSION))", remote_build)
        self.assertIn("shutil.copy2(archive_output, local_output)", remote_build)
        for script in (local_wsl, ubuntu):
            self.assertIn("next_archive_path()", script)
            self.assertIn('ARCHIVE_OUT="$(next_archive_path "$LOCAL_OUT" "v4.4.128")"', script)
            self.assertIn('cp "$ARCHIVE_OUT" "$LOCAL_OUT"', script)

    def test_four_board_test_cleans_saved_topology_before_configuring_roles(self):
        tool_source = (self.REPO_ROOT / "automation/ws63/tools/ws63_four_board_relay_test.py").read_text(
            encoding="utf-8"
        )

        self.assertIn("def _clean_start_saved_config", tool_source)
        self.assertIn('command="cfg clear"', tool_source)
        self.assertIn('key="runtimeConfigured"', tool_source)
        self.assertIn("expected=False", tool_source)
        self.assertIn("if not args.no_clean_start:", tool_source)
        clean_index = tool_source.index("if not args.no_clean_start:")
        configure_call_index = tool_source.index("_configure_roles(", clean_index)
        self.assertLess(clean_index, configure_call_index)
        self.assertIn('--no-clean-start', tool_source)

    def test_serial_members_include_mac_suffix_label_for_operator_identity(self):
        cli_source = (self.REPO_ROOT / "src/sle_team_cli.c").read_text(encoding="utf-8")
        member_label = self._extract_c_function(cli_source, "static void sle_team_cli_format_member_label")
        cli_handle = self._extract_c_function(cli_source, "void sle_team_cli_handle_line")

        self.assertIn('"M%02X%02X"', member_label)
        self.assertIn('"member=%u label=%s role=%u online=%u', cli_handle)
        self.assertIn("mac=%02X%02X ready=%u", cli_handle)
        self.assertIn('"pending member=%u role=%u battery=%u mac=%02X%02X ready=%u last_seen=%lu label=%s"', cli_handle)

    def test_display_flush_is_isolated_from_team_network_task(self):
        app_source = (self.REPO_ROOT / "xc/ws63_team_network/src/ws63_team_network_app.c").read_text(encoding="utf-8")
        common_tick = self._extract_c_function(app_source, "static void team_network_tick_common")
        display_task = self._extract_c_function(app_source, "static void *team_display_task")
        app_stack = self._extract_define_int(app_source, "SLE_TEAM_APP_TASK_STACK_SIZE")
        display_stack = self._extract_define_int(app_source, "SLE_TEAM_DISPLAY_TASK_STACK_SIZE")
        app_prio = self._extract_define_int(app_source, "SLE_TEAM_APP_TASK_PRIO")
        display_prio = self._extract_define_int(app_source, "SLE_TEAM_DISPLAY_TASK_PRIO")

        self.assertNotIn("ws63_st7789_tick()", common_tick)
        self.assertNotIn("team_display_flush_pending_once()", common_tick)
        self.assertIn("ws63_st7789_tick();", display_task)
        self.assertIn("team_display_flush_pending_once();", display_task)
        self.assertIn("osal_msleep(SLE_TEAM_DISPLAY_TASK_INTERVAL_MS);", display_task)
        self.assertGreaterEqual(display_stack, app_stack)
        self.assertGreater(display_prio, app_prio)

    def test_display_events_cache_mac_suffix_labels_before_route_id_fallback(self):
        app_source = (self.REPO_ROOT / "xc/ws63_team_network/src/ws63_team_network_app.c").read_text(encoding="utf-8")
        runtime_struct = re.search(r"typedef struct \{[\s\S]*?\n\} sle_team_ws63_runtime_t;", app_source)
        format_label = self._extract_c_function(app_source, "static void team_display_format_member_label")
        show_event = self._extract_c_function(app_source, "static void team_display_show_event")

        self.assertIsNotNone(runtime_struct)
        self.assertIn("char display_member_labels[SLE_TEAM_MAX_MEMBERS][8];", runtime_struct.group(0))
        self.assertIn("static void team_display_cache_member_label", app_source)
        self.assertIn("static uint8_t team_display_get_cached_member_label", app_source)
        self.assertIn("pending = team_display_find_pending_member_record(member_id);", format_label)
        self.assertIn("team_display_cache_member_label(member_id, out);", format_label)
        self.assertIn("team_display_get_cached_member_label(member_id, out, out_size)", format_label)
        self.assertLess(
            format_label.index("member->mac_ready != 0U"),
            format_label.index("team_display_get_cached_member_label(member_id, out, out_size)"),
        )
        self.assertLess(
            format_label.index("team_display_get_cached_member_label(member_id, out, out_size)"),
            format_label.rindex("team_identity_format_route_label(member_id"),
        )
        self.assertIn("team_display_format_member_label(member_id, label, sizeof(label));", show_event)
        self.assertIn("[display-event] event=%s label=%s member=%u", app_source)

    def test_leader_direct_next_hop_routes_use_physical_conn_id(self):
        app_source = (self.REPO_ROOT / "xc/ws63_team_network/src/ws63_team_network_app.c").read_text(encoding="utf-8")
        route_find = self._extract_c_function(app_source, "static uint8_t team_route_find")

        self.assertIn("team_route_next_hop_is_direct_peer(member_id, route->next_hop_id) == 0U", route_find)
        self.assertIn("sle_uart_client_find_conn_by_member(route->next_hop_id", route_find)
        self.assertIn("team_route_conn_is_active(dir, route->conn_id)", route_find)
        self.assertLess(
            route_find.index("team_route_next_hop_is_direct_peer(member_id, route->next_hop_id) == 0U"),
            route_find.index("sle_uart_client_find_conn_by_member(route->next_hop_id"),
        )
        self.assertLess(
            route_find.index("sle_uart_client_find_conn_by_member(route->next_hop_id"),
            route_find.index("team_route_conn_is_active(dir, route->conn_id)"),
        )
        self.assertNotIn("route->next_hop_id != 0U && route->next_hop_id != SLE_TEAM_BROADCAST_ID", route_find)

    def test_leader_bind_packet_source_preserves_physical_first_hop(self):
        app_source = (self.REPO_ROOT / "xc/ws63_team_network/src/ws63_team_network_app.c").read_text(encoding="utf-8")
        bind_source = self._extract_c_function(app_source, "static void team_bind_packet_source")

        self.assertIn("sle_team_route_update_body_t route_update;", bind_source)
        self.assertIn("memcpy(&route_update, app_packet.body, sizeof(route_update))", bind_source)
        self.assertIn("conn_dir = dir == TEAM_LINK_DOWNSTREAM ? TEAM_CONN_DIR_DOWNSTREAM : TEAM_CONN_DIR_UPSTREAM", bind_source)
        self.assertIn("g_team_node.cfg.role != SLE_TEAM_ROLE_LEADER", bind_source)
        self.assertIn("team_conn_track_route_id_is_valid(track) != 0U", bind_source)
        self.assertIn("app_packet.src_id is the logical origin", bind_source)
        self.assertIn("track->route_id_provisional != 0U && track->route_id == app_packet.src_id", bind_source)
        self.assertIn("route_update_can_override_next_hop", bind_source)
        self.assertIn("physical_peer_id != app_packet.src_id", bind_source)
        self.assertIn("route update keep physical next_hop", bind_source)
        self.assertIn(
            "physical_peer_id = team_conn_track_route_id_is_valid(track) != 0U ? track->route_id : app_packet.src_id",
            bind_source,
        )
        self.assertIn("sle_uart_client_bind_member_conn(physical_peer_id, conn_id);", bind_source)
        self.assertIn(
            "team_route_note(app_packet.src_id, conn_id, TEAM_CONN_DIR_DOWNSTREAM, next_hop_id);",
            bind_source,
        )
        self.assertLess(
            bind_source.index("if (g_team_node.cfg.role == SLE_TEAM_ROLE_LEADER)"),
            bind_source.index("sle_uart_client_bind_member_conn(physical_peer_id, conn_id);"),
        )
        self.assertLess(
            bind_source.index("sle_uart_client_bind_member_conn(physical_peer_id, conn_id);"),
            bind_source.index("} else if (dir == TEAM_LINK_DOWNSTREAM)"),
        )

    def test_route_update_observers_copy_packet_body_before_reading(self):
        app_source = (self.REPO_ROOT / "xc/ws63_team_network/src/ws63_team_network_app.c").read_text(encoding="utf-8")
        observe_source = self._extract_c_function(app_source, "static void team_route_update_observe")
        bind_source = self._extract_c_function(app_source, "static void team_bind_packet_source")

        self.assertIn("sle_team_route_update_body_t route_update;", observe_source)
        self.assertIn("memcpy(&route_update, app_packet->body, sizeof(route_update))", observe_source)
        self.assertIn("sle_team_route_update_body_t route_update;", bind_source)
        self.assertIn("memcpy(&route_update, app_packet.body, sizeof(route_update))", bind_source)
        self.assertNotIn("(const sle_team_route_update_body_t *)", observe_source)
        self.assertNotIn("(const sle_team_route_update_body_t *)", bind_source)

    def test_four_board_log_gate_catches_route_hint_no_route_regression(self):
        leader = fb.lc.Peer(name="leader", port="COML", baudrate=115200, ser=FakeSerial())
        leader.log.append(
            "[sle-tx-fail] type=PACKET dst=86 ret=-4 reason=NO_ROUTE\r\n"
            "[team] route hint member=86 parent=241 ret=-4\r\n"
        )

        events = fb._find_route_regression_events([leader], leader_id=154)

        self.assertEqual(
            events,
            [
                "leader: route hint failed member=86 parent=241 ret=-4",
                "leader: route hint send hit NO_ROUTE dst=86",
            ],
        )

    def test_four_board_log_gate_catches_relay_leader_bound_no_route_regression(self):
        relay = fb.lc.Peer(name="relay", port="COMR", baudrate=115200, ser=FakeSerial())
        relay.log.append(
            "[sle-rx] HELLO 241->154 seq=11\r\n"
            "[sle-tx-fail] type=PACKET dst=154 ret=-4 reason=NO_ROUTE\r\n"
        )

        events = fb._find_route_regression_events([relay], leader_id=154)

        self.assertEqual(events, ["relay: leader-bound packet hit NO_ROUTE dst=154"])

    def test_four_board_log_gate_catches_relay_dropping_child_hello_before_forward(self):
        relay = fb.lc.Peer(name="relay", port="COMR", baudrate=115200, ser=FakeSerial())
        relay.log.append(
            "[sle-rx] HELLO 224->154 seq=9\r\n"
            "[team] node packet role=0 len=26 ret=-4\r\n"
        )

        events = fb._find_route_regression_events([relay], leader_id=154)

        self.assertEqual(events, ["relay: leader-bound packet was rejected before relay forward"])

    def test_four_board_log_gate_tolerates_bootstrap_reject_followed_by_forward(self):
        relay = fb.lc.Peer(name="relay", port="COMR", baudrate=115200, ser=FakeSerial())
        relay.log.append(
            "[sle-rx] HELLO 86->154 seq=38\r\n"
            "[team] node packet role=0 len=26 ret=-4\r\n"
            "[sle-rx] HELLO 86->154 seq=39\r\n"
            "[state] relay forwarded packet\r\n"
        )

        events = fb._find_route_regression_events([relay], leader_id=154)

        self.assertEqual(events, [])

    def test_child_reboot_gate_accepts_rejoin_without_offline_timeout(self):
        leader = fb.lc.Peer(name="leader", port="COML", baudrate=115200, ser=FakeSerial())
        peers = [leader]
        original_query = fb._query_records_once

        def fake_query_records_once(_leader, _peers, window_s=1.0):
            leader.log.append("[sle-rx] HELLO 224->154 seq=2\r\n[team] joined member=224\r\n")
            return {224: {"online": 1, "relay": 0, "tier": 0, "max_down": 0, "last_seen": 12}}

        try:
            fb._query_records_once = fake_query_records_once
            records = fb._wait_leader_observes_member_reboot_rejoin(
                leader,
                peers,
                member_id=224,
                leader_id=154,
                timeout_s=1.0,
                poll_s=0.2,
                log_start=0,
            )
        finally:
            fb._query_records_once = original_query

        self.assertEqual(records[224]["online"], 1)

    def test_final_topology_accepts_forwarding_evidence_when_metrics_do_not_refresh(self):
        leader = fb.lc.Peer(name="leader", port="COML", baudrate=115200, ser=FakeSerial())
        leader.log.append(
            "[sle uart client] bind member:224 conn_id:1\r\n"
            "[sle-rx] HEARTBEAT 241->154 seq=90\r\n"
            "[sle uart client] bind member:224 conn_id:1\r\n"
            "[sle-rx] HEARTBEAT 224->154 seq=101\r\n"
            "[sle uart client] bind member:224 conn_id:1\r\n"
            "[sle-rx] HEARTBEAT 86->154 seq=117\r\n"
        )
        peers = [leader]
        original_query = fb._query_records_once

        def fake_query_records_once(_leader, _peers, window_s=1.0):
            return {
                241: {"online": 1, "relay": 0, "tier": 0, "max_down": 0, "last_seen": 10},
                224: {"online": 1, "relay": 1, "tier": 2, "max_down": 8, "last_seen": 10},
                86: {"online": 1, "relay": 0, "tier": 0, "max_down": 0, "last_seen": 10},
            }

        try:
            fb._query_records_once = fake_query_records_once
            records = fb._wait_stable_final_topology(
                leader,
                peers,
                leader_id=154,
                member_ids=[241, 224, 86],
                direct_cap=1,
                timeout_s=1.0,
                poll_s=0.05,
                stable_polls=2,
            )
        finally:
            fb._query_records_once = original_query

        self.assertEqual(records[224]["relay"], 1)
        self.assertEqual(records[241]["relay"], 0)

    def test_parser_exposes_natural_default_capacity_options(self):
        args = fb.build_parser().parse_args(["--skip-direct-config", "--natural-members"])

        self.assertTrue(args.skip_direct_config)
        self.assertTrue(args.natural_members)

    def test_natural_mode_waits_pending_and_skips_manual_approve(self):
        tool_source = (self.REPO_ROOT / "automation/ws63/tools/ws63_four_board_relay_test.py").read_text(
            encoding="utf-8"
        )

        natural_branch = tool_source[
            tool_source.index("if args.natural_members:")
            : tool_source.index("else:", tool_source.index("if args.natural_members:"))
        ]
        self.assertIn("pairing stop lets firmware auto-policy decide", natural_branch)
        self.assertIn("_wait_leader_sees_member", natural_branch)
        self.assertNotIn("_approve_member", natural_branch)

    def test_final_topology_accepts_default_direct_no_relay_without_metrics(self):
        leader = fb.lc.Peer(name="leader", port="COML", baudrate=115200, ser=FakeSerial())
        peers = [leader]
        original_query = fb._query_records_once

        def fake_query_records_once(_leader, _peers, window_s=1.0):
            return {
                241: {"online": 1, "relay": 0, "tier": 0, "max_down": 0, "last_seen": 10},
                224: {"online": 1, "relay": 0, "tier": 0, "max_down": 0, "last_seen": 10},
                86: {"online": 1, "relay": 0, "tier": 0, "max_down": 0, "last_seen": 10},
            }

        try:
            fb._query_records_once = fake_query_records_once
            records = fb._wait_stable_final_topology(
                leader,
                peers,
                leader_id=154,
                member_ids=[241, 224, 86],
                direct_cap=8,
                timeout_s=1.0,
                poll_s=0.05,
                stable_polls=2,
                allow_any_converged=True,
            )
        finally:
            fb._query_records_once = original_query

        self.assertEqual(fb._relay_ids_from_records(records, [241, 224, 86]), [])

    def test_natural_mode_uses_member_table_to_select_actual_relay(self):
        tool_source = (self.REPO_ROOT / "automation/ws63/tools/ws63_four_board_relay_test.py").read_text(
            encoding="utf-8"
        )

        self.assertEqual(
            fb._prefer_non_relay_member_id(
                {
                    241: {"online": 1, "relay": 1},
                    224: {"online": 1, "relay": 0},
                    86: {"online": 1, "relay": 0},
                },
                [241, 224, 86],
            ),
            224,
        )
        self.assertIn("natural_relay_id = post_member_relays[0]", tool_source)
        self.assertIn("natural_relay = peer_by_member_id[natural_relay_id]", tool_source)
        self.assertIn("natural direct policy: no relay elected", tool_source)

    def test_final_topology_route_metrics_window_starts_at_relay_reboot(self):
        tool_source = (self.REPO_ROOT / "automation/ws63/tools/ws63_four_board_relay_test.py").read_text(
            encoding="utf-8"
        )

        self.assertIn("relay_offline_start = len(leader.log)", tool_source)
        self.assertIn("final_route_log_start = relay_offline_start", tool_source)
        self.assertNotIn("final_route_log_start = len(leader.log)", tool_source)

    def test_four_board_policy_summary_reports_original_vs_child_relay_outcome(self):
        self.assertEqual(
            fb._summarize_policy(
                {
                    13: {"online": 1, "relay": 1, "tier": 1, "max_down": 8, "last_seen": 10},
                    16: {"online": 1, "relay": 0, "tier": 0, "max_down": 0, "last_seen": 10},
                    17: {"online": 1, "relay": 0, "tier": 0, "max_down": 0, "last_seen": 10},
                },
                relay_id=13,
                child_ids=[16, 17],
            ),
            "original relay regained relay role; child relay was demoted",
        )
        self.assertEqual(
            fb._summarize_policy(
                {
                    13: {"online": 1, "relay": 0, "tier": 0, "max_down": 0, "last_seen": 10},
                    16: {"online": 1, "relay": 1, "tier": 1, "max_down": 8, "last_seen": 10},
                    17: {"online": 1, "relay": 0, "tier": 0, "max_down": 0, "last_seen": 10},
                },
                relay_id=13,
                child_ids=[16, 17],
            ),
            "new child relay retained role; original relay returned as member; child_relays=[16]",
        )

    def test_firmware_relay_recovery_policy_is_target_based(self):
        app_source = (self.REPO_ROOT / "xc/ws63_team_network/src/ws63_team_network_app.c").read_text(encoding="utf-8")

        self.assertIn("static uint8_t team_leader_relay_target_for_member_count(uint8_t member_count)", app_source)
        self.assertIn("downstream_count = (uint8_t)(member_count - direct_capacity);", app_source)
        self.assertIn("target = team_ceil_div_u16_to_u8(downstream_count, (uint16_t)SLE_TEAM_MAX_DIRECT_CONNECTIONS);", app_source)
        self.assertIn("relay_budget = team_leader_relay_budget();", app_source)
        self.assertIn("budget = team_min_u8(budget, team_leader_relay_budget_by_ram());", app_source)
        self.assertIn("budget = team_min_u8(budget, direct_capacity);", app_source)
        self.assertIn("team_leader_known_member_count", app_source)
        self.assertIn("relay_target = team_leader_relay_target_with_failover(demand_count, relay_count, now_s);", app_source)
        self.assertIn("target=%u budget=%u", app_source)
        self.assertIn("while (relay_count > relay_target)", app_source)
        self.assertIn("team_leader_pick_worst_active_relay(now_s, timeout_s)", app_source)
        self.assertIn('"auto-demote"', app_source)
        self.assertIn("while (relay_count < relay_target)", app_source)
        self.assertIn("team_leader_pick_best_relay_candidate(now_s, timeout_s)", app_source)
        self.assertIn('"auto-promote"', app_source)
        self.assertIn("team_leader_failover_begin(member_id, now_s)", app_source)
        self.assertIn("team_leader_failover_watch_member(route->member_id)", app_source)
        self.assertIn(
            "if (target == 0U && relay_count != 0U && team_leader_failover_window_active(now_s) != 0U)",
            app_source,
        )

    def test_pairing_stop_preserves_existing_online_members_in_allowlist(self):
        node_source = (self.REPO_ROOT / "src/sle_team_node.c").read_text(encoding="utf-8")
        pairing_stop = self._extract_c_function(node_source, "int sle_team_node_pairing_stop")

        self.assertIn("Preserve members that were already connected before the pairing window", pairing_stop)
        self.assertIn("uint8_t known_ids[SLE_TEAM_MAX_MEMBERS];", pairing_stop)
        self.assertIn("node->members[i].member_id", pairing_stop)
        self.assertIn("known_ids[known_count++] = node->members[i].member_id;", pairing_stop)
        self.assertIn("sle_team_node_add_allowed_member(node, known_ids[i])", pairing_stop)
        self.assertLess(
            pairing_stop.index("sle_team_node_add_allowed_member(node, known_ids[i])"),
            pairing_stop.index("sle_team_node_pairing_approve_with_relay(node, pending_ids[i], 0U)"),
        )

    def test_dynamic_relay_budget_capacity_examples(self):
        app_source = (self.REPO_ROOT / "xc/ws63_team_network/src/ws63_team_network_app.c").read_text(encoding="utf-8")
        header_source = (self.REPO_ROOT / "include/sle_team_node.h").read_text(encoding="utf-8")

        max_members = self._extract_define_int(header_source, "SLE_TEAM_MAX_LOGICAL_MEMBERS")
        fanout = self._extract_define_int(header_source, "SLE_TEAM_MAX_DIRECT_CONNECTIONS")
        ram_budget = self._extract_define_int(app_source, "SLE_TEAM_RELAY_MGMT_RAM_BUDGET_BYTES")
        bytes_per_relay = self._extract_define_int(app_source, "SLE_TEAM_RELAY_MGMT_EST_BYTES_PER_RELAY")
        hard_max = fanout

        def relay_budget(member_limit: int, direct_cap: int) -> int:
            if member_limit <= direct_cap:
                return 0
            table_budget = math.ceil((member_limit - direct_cap) / fanout)
            ram_limited = ram_budget // bytes_per_relay
            return min(table_budget, ram_limited, hard_max, direct_cap)

        def relay_target(member_count: int, direct_cap: int, member_limit: int = max_members) -> int:
            budget = relay_budget(member_limit, direct_cap)
            if member_count <= direct_cap or budget == 0:
                return 0
            target = math.ceil((member_count - direct_cap) / fanout)
            if direct_cap >= fanout and target < 2 and budget >= 2:
                target = 2
            return min(target, budget)

        self.assertEqual(max_members, 30)
        self.assertEqual(relay_budget(max_members, 8), 3)
        self.assertEqual(relay_target(30, 8), 3)
        self.assertEqual(relay_budget(max_members, 1), 1)
        self.assertEqual(relay_target(3, 1), 1)
        self.assertEqual(relay_budget(64, 8), 7)


if __name__ == "__main__":
    unittest.main()
