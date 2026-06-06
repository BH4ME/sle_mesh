import assert from "node:assert/strict";
import fs from "node:fs";
import path from "node:path";
import test from "node:test";
import { fileURLToPath } from "node:url";

/*
 * Structural contract tests:
 * Some assertions intentionally match source snippets to lock behavior/shape.
 * If formatting/refactor changes these snippets, update the regex accordingly.
 */
const repoRoot = path.resolve(fileURLToPath(new URL("../..", import.meta.url)));
const readRepoText = (...segments) =>
  fs.readFileSync(path.join(repoRoot, ...segments), "utf8").replace(/\r\n/g, "\n");
const contract = JSON.parse(fs.readFileSync(path.join(repoRoot, "webui/shared/ws63-api.json"), "utf8"));
const webPackage = JSON.parse(fs.readFileSync(path.join(repoRoot, "webui/package.json"), "utf8"));
const viteConfigSource = readRepoText("webui/vite.config.ts");
const webReadmeSource = readRepoText("webui/README.md");
const rootReadmeSource = readRepoText("README.md");
const versionsReadmeSource = readRepoText("versions/README.md");
const projectSopSource = readRepoText("meta/PROJECT_OPERATION_SOP.md");
const buildScriptSource = readRepoText("scripts/ws63_build_v4_ubuntu.sh");
const firmwareSource = readRepoText("xc/ws63_team_network/src/ws63_team_network_app.c");
const webApiSource = readRepoText("src/sle_team_web_api.c");
const mainSource = readRepoText("webui/src/main.ts");
const clientSource = readRepoText("webui/src/api/client.ts");
const firmwareKconfigSource = readRepoText("xc/ws63_team_network/Kconfig");
const displaySource = readRepoText("xc/ws63_team_network/src/ws63_st7789_display.c");
const lvConfSource = readRepoText("xc/ws63_team_network/lv_conf.h");
const nodeSource = readRepoText("src/sle_team_node.c");
const cmakeSource = readRepoText("xc/ws63_team_network/CMakeLists.txt");
const gitmodulesPath = path.join(repoRoot, ".gitmodules");
const gitmodulesSource = fs.existsSync(gitmodulesPath) ? readRepoText(".gitmodules") : "";
const lvglPatchPath = path.join(
  repoRoot,
  "xc/ws63_team_network/third_party/lvgl-patches/lv8.3.11-ws63-c89-rect.patch",
);
const lvglPatchSource = fs.existsSync(lvglPatchPath)
  ? readRepoText("xc/ws63_team_network/third_party/lvgl-patches/lv8.3.11-ws63-c89-rect.patch")
  : "";
const sleUartClientSource = readRepoText("xc/ws63_team_network/sle_uart_client/sle_uart_client.c");
const sleUartServerSource = readRepoText("xc/ws63_team_network/sle_uart_server/sle_uart_server.c");
const serialCfgSource = readRepoText("scripts/ws63_serial_cfg.ps1");

test("WS63 API contract lists the current board HTTP routes", () => {
  assert.deepEqual(
    contract.routes.map((route) => route.path),
    [
      "/api/status",
      "/api/nodes",
      "/api/events",
      "/api/pending",
      "/api/location",
      "/api/config/status",
      "/api/config/leader",
      "/api/config/member",
      "/api/config/apply",
      "/api/config/clear",
      "/api/config/reboot",
      "/api/role",
      "/api/pairing",
      "/api/member/select",
      "/api/member/leave",
      "/api/factory-reset",
    ],
  );
});

test("contract routes are implemented by the burned firmware WebUI", () => {
  for (const route of contract.routes) {
    assert.match(
      firmwareSource,
      new RegExp(`GET ${route.path.replace(/[.*+?^${}()|[\]\\]/g, "\\$&")}`),
      `${route.path} missing from ws63_team_network_app.c`,
    );
  }
});

test("firmware exposes unified config over HTTP and serial", () => {
  assert.match(firmwareSource, /GET \/api\/config\/status/);
  assert.match(firmwareSource, /GET \/api\/config\/leader/);
  assert.match(firmwareSource, /GET \/api\/config\/member/);
  assert.match(firmwareSource, /GET \/api\/config\/apply/);
  assert.match(firmwareSource, /GET \/api\/config\/clear/);
  assert.match(firmwareSource, /GET \/api\/config\/reboot/);
  assert.match(firmwareSource, /\[cfg-json\]/);
  assert.match(firmwareSource, /team_cfg_status_write_json/);
});

test("current firmware makes ST7789 member events readable and panel-styled", () => {
  assert.match(firmwareSource, /#define SLE_TEAM_FW_VERSION "v4\.4\.95"/);
  assert.match(firmwareSource, /#define SLE_TEAM_HW_CONSTRAINTS "v4\.4\.95 pairing allowlist preserve"/);
  assert.match(displaySource, /SLE\/\/BOOT/);
  assert.match(displaySource, /LINK-MESH/);
  assert.match(displaySource, /g_st7789_lv_panel_status/);
  assert.match(displaySource, /g_st7789_lv_panel_event/);
  assert.match(displaySource, /g_st7789_lv_event_rail/);
  assert.match(displaySource, /LV_LABEL_LONG_WRAP/);
  assert.match(displaySource, /NODE ONLINE/);
  assert.match(displaySource, /MANUAL LEAVE/);
  assert.match(displaySource, /HEARTBEAT T\/O/);
  assert.match(displaySource, /BACK ONLINE/);
  assert.match(lvConfSource, /#define LV_MEM_SIZE \(20U \* 1024U\)/);
  assert.match(displaySource, /int ws63_st7789_show_event\(uint8_t event, const char \*member_label/);
  assert.doesNotMatch(displaySource, /ws63_st7789_show_alert/);
  assert.doesNotMatch(displaySource, /LOST M%u/);
  assert.match(firmwareSource, /TEAM_DISPLAY_EVENT_JOIN/);
  assert.match(firmwareSource, /TEAM_DISPLAY_EVENT_LEFT/);
  assert.match(firmwareSource, /TEAM_DISPLAY_EVENT_TIMEOUT/);
  assert.match(firmwareSource, /TEAM_DISPLAY_EVENT_LOST/);
  assert.match(firmwareSource, /TEAM_DISPLAY_EVENT_REJOIN/);
  assert.match(firmwareSource, /team_display_format_member_label/);
  assert.match(firmwareSource, /team_display_note_member_joined/);
  assert.match(firmwareSource, /team_display_note_member_offline/);
  assert.match(firmwareSource, /team_display_event_name/);
  assert.match(firmwareSource, /\[display-event\] event=%s label=%s member=%u/);
  assert.match(firmwareSource, /team_identity_format_route_label\(member->member_id, member->role, member->mac/);
  assert.match(rootReadmeSource, /Current repo record: `v4\.4\.95`/);
  assert.match(rootReadmeSource, /Current firmware version: `v4\.4\.95`/);
  assert.match(rootReadmeSource, /meta\/PROJECT_OPERATION_SOP\.md/);
  assert.match(rootReadmeSource, /versions\/v4\.4\.95\/VERSION\.md/);
  assert.match(versionsReadmeSource, /- \[v4\.4\.95\]\(\.\/v4\.4\.95\/VERSION\.md\)/);
  assert.match(versionsReadmeSource, /- \[v4\.4\.94\]\(\.\/v4\.4\.94\/VERSION\.md\)/);
  assert.match(versionsReadmeSource, /- \[v4\.4\.93\]\(\.\/v4\.4\.93\/VERSION\.md\)/);
  assert.match(versionsReadmeSource, /- \[v4\.4\.92\]\(\.\/v4\.4\.92\/VERSION\.md\)/);
  assert.match(versionsReadmeSource, /- \[v4\.4\.91\]\(\.\/v4\.4\.91\/VERSION\.md\)/);
  assert.match(versionsReadmeSource, /- \[v4\.4\.90\]\(\.\/v4\.4\.90\/VERSION\.md\)/);
  assert.match(versionsReadmeSource, /- \[v4\.4\.89\]\(\.\/v4\.4\.89\/VERSION\.md\)/);
  assert.match(versionsReadmeSource, /- \[v4\.4\.88\]\(\.\/v4\.4\.88\/VERSION\.md\)/);
  assert.match(versionsReadmeSource, /- \[v4\.4\.76\]\(\.\/v4\.4\.76\/VERSION\.md\)/);
  assert.match(versionsReadmeSource, /- \[v4\.4\.75\]\(\.\/v4\.4\.75\/VERSION\.md\)/);
  assert.match(projectSopSource, /每次代码行为变化都必须升版本/);
  assert.match(projectSopSource, /远程 Ubuntu 编译/);
  assert.match(projectSopSource, /自动烧录/);
});

test("v4.4.61 keeps runtime direct-cap relay route planning synchronized", () => {
  assert.match(firmwareSource, /cfg direct %u/);
  assert.match(firmwareSource, /runtimeDirectCap/);
  assert.match(firmwareSource, /runtimeRelayBudget/);
  assert.match(firmwareSource, /static uint8_t team_leader_direct_capacity\(void\)/);
  assert.match(firmwareSource, /static uint8_t team_leader_relay_budget\(void\)/);
  assert.match(firmwareSource, /SLE_TEAM_RELAY_MGMT_RAM_BUDGET_BYTES/);
  assert.match(
    firmwareSource,
    /team_leader_relay_target_for_member_count[\s\S]+direct_capacity = team_leader_direct_capacity\(\)/,
  );
  assert.match(firmwareSource, /known_count = team_leader_known_member_count\(\)/);
  assert.match(firmwareSource, /demand_count = known_count > online_count \? known_count : online_count/);
  assert.match(firmwareSource, /relay rebalance demand online=%u known=%u pending=%u demand=%u direct_cap=%u relay=%u target=%u budget=%u/);
  assert.match(firmwareSource, /SLE_TEAM_RELAY_SWAP_RSSI_HYST_DBM 8/);
  assert.match(firmwareSource, /SLE_TEAM_RELAY_SWAP_STABLE_S 30U/);
  assert.match(firmwareSource, /relay_swap_candidate_id/);
  assert.match(firmwareSource, /relay_swap_victim_id/);
  assert.match(firmwareSource, /relay_swap_since_s/);
  assert.match(firmwareSource, /static uint8_t team_leader_relay_swap_tick/);
  assert.match(firmwareSource, /static uint8_t team_leader_desired_parent_for_member/);
  assert.match(firmwareSource, /static uint8_t team_leader_route_parent_matches/);
  assert.match(firmwareSource, /plan=%u/);
  assert.match(firmwareSource, /static uint8_t team_member_find_upstream_parent_conn/);
  assert.match(firmwareSource, /team_member_find_upstream_parent_conn\(&target_conn_id\)/);
  assert.match(firmwareSource, /sle_uart_client_bind_member_conn\(physical_peer_id,\s*conn_id\)/);
  assert.match(clientSource, /runtimeDirectCap: numberField\("runtimeDirectCap"\)/);
  assert.match(clientSource, /runtimeRelayBudget: numberField\("runtimeRelayBudget"\)/);
  assert.match(webApiSource, /relayTarget\\":%u[\s\S]+relayOnline\\":%u[\s\S]+relayBudget\\":%u/);
  assert.match(clientSource, /maxDownstream: config\.runtimeDirectCap/);
});

test("v4.4.61 treats WS63 SLE conn_id zero as a valid route connection", () => {
  const routeConnBlock =
    firmwareSource.match(/static uint8_t team_route_conn_is_active[\s\S]+?\n}\n/)?.[0] ?? "";

  assert.match(routeConnBlock, /WS63 SLE reports conn_id 0 as a valid ACL connection/);
  assert.doesNotMatch(routeConnBlock, /conn_id\s*==\s*0U/);
  assert.match(routeConnBlock, /return sle_uart_client_has_conn\(conn_id\)/);
  assert.match(routeConnBlock, /return sle_uart_server_has_conn\(conn_id\)/);
});

test("v4.4.58 keeps disconnect-to-display identity and makes LOST idempotent", () => {
  assert.match(firmwareSource, /static uint8_t team_route_member_by_conn\(uint16_t conn_id/);
  assert.match(firmwareSource, /uint8_t tracked_member_id = 0U;/);
  assert.match(firmwareSource, /uint8_t routed_member_id = 0U;/);
  assert.match(firmwareSource, /display_member_last_events\[SLE_TEAM_MAX_MEMBERS\]/);
  assert.match(firmwareSource, /static uint8_t team_display_member_last_event\(uint8_t member_id/);
  assert.match(firmwareSource, /team_route_member_by_conn\(conn_id, dir, &routed_member_id\)/);
  assert.match(
    firmwareSource,
    /tracked_member_id = track->route_id;[\s\S]+team_conn_track_update\(conn_id, dir, addr\)/,
  );
  assert.match(
    firmwareSource,
    /team_leader_mark_member_offline\(disconnected_member_id, "conn_disconnected"\)/,
  );
  assert.match(firmwareSource, /\[team\] disconnect lookup conn=%u dir=%u known=%u member=%u tracked=%u routed=%u/);
  assert.match(firmwareSource, /already_offline = 1U;/);
  assert.match(firmwareSource, /reason=conn_disconnected already_offline=1 last_event=LEFT/);
  assert.match(firmwareSource, /member offline id=%u reason=%s last_seen=%lu already_offline=%u last_event=%u/);
  assert.match(firmwareSource, /was_relay = already_offline == 0U \? member->relay_allowed : 0U;/);
});

test("protocol separates manual leave from link-lost auto recovery", () => {
  assert.match(nodeSource, /alert\.reason = SLE_TEAM_ALERT_LEAVE;/);
  assert.match(nodeSource, /node->state = SLE_TEAM_NET_IDLE;/);
  assert.match(nodeSource, /node->cfg\.leader_id = 0U;/);
  assert.match(nodeSource, /int sle_team_node_member_link_lost\(sle_team_node_t \*node\)/);
  assert.match(nodeSource, /return sle_team_member_recover_link\(node, "link lost, rejoining"\);/);
  assert.match(
    nodeSource,
    /static int sle_team_member_recover_link[\s\S]+node->state = SLE_TEAM_NET_DISCOVERING;[\s\S]+sle_team_node_disable_member_relay\(node,\s*0U\);/,
  );
  assert.match(
    nodeSource,
    /node->cfg\.leader_id != 0U[\s\S]+node->state != SLE_TEAM_NET_IDLE[\s\S]+sle_team_node_send_hello\(node,\s*node->cfg\.leader_id\);/,
  );
});

test("v4.4.37 keeps board HTTP WebUI auto-start enabled by default", () => {
  const wifiTask = firmwareSource.match(/static void \*team_wifi_ap_task[\s\S]+?\n}\n/)?.[0] ?? "";
  assert.match(firmwareSource, /#define CONFIG_SLE_TEAM_WIFI_AP_AUTO_START 1/);
  assert.match(
    firmwareKconfigSource,
    /config SLE_TEAM_WIFI_AP_AUTO_START\s+bool "Auto-start WiFi SoftAP and HTTP WebUI"\s+default y\s+depends on SLE_TEAM_WIFI_AP_ENABLE/,
  );
  assert.match(buildScriptSource, /set_kconfig_value\(s,\s*"CONFIG_SLE_TEAM_WIFI_AP_AUTO_START",\s*"y"\)/);
  assert.match(buildScriptSource, /python3 build\.py -c ws63-liteos-app -j'\$BUILD_JOBS'/);
  assert.match(wifiTask, /#if !CONFIG_SLE_TEAM_WIFI_AP_AUTO_START[\s\S]+return NULL;[\s\S]+#endif/s);
  assert.match(wifiTask, /team_tcpip_init_wait\(\)/);
  assert.match(wifiTask, /team_http_server_loop\(\)/);
});

test("v4.4.30 restores leader reboot membership using persisted allowlist and fixed SLE profile", () => {
  assert.match(firmwareSource, /#define SLE_TEAM_NV_KEY_ALLOWED_MEMBERS 0x5002/);
  assert.match(firmwareSource, /SLE_TEAM_NV_ALLOWED_MAGIC/);
  assert.match(firmwareSource, /static int team_nv_allowed_save_from_node\(void\)/);
  assert.match(firmwareSource, /static int team_nv_allowed_apply_to_node\(void\)/);
  assert.match(firmwareSource, /team_nv_allowed_apply_to_node\(\);[\s\S]+restore leader ret=/);
  assert.match(firmwareSource, /pairing approve[\s\S]+team_nv_allowed_save_from_node\(\)/);
  assert.match(sleUartClientSource, /#define SLE_UART_DEFAULT_PROPERTY_HANDLE 2U/);
  assert.match(sleUartClientSource, /sle_uart_client_mark_ready\(SLE_UART_DEFAULT_PROPERTY_HANDLE,\s*"fixed-profile"\)/);
  assert.match(sleUartClientSource, /if \(property == NULL \|\| status != ERRCODE_SLE_SUCCESS\)/);
});

test("v4.4.47 treats nv flush warnings as non-fatal after successful writes", () => {
  const writeOnlyNvReturns = [
    ...firmwareSource.matchAll(/return ret == ERRCODE_SUCC \? SLE_TEAM_OK : SLE_TEAM_ERR_UNSUPPORTED;/g),
  ];
  assert.match(firmwareSource, /flush_ret = uapi_nv_flush\(\)/);
  assert.match(firmwareSource, /ret=0x%x flush=0x%x/);
  assert.ok(
    writeOnlyNvReturns.length >= 3,
    "config save, config clear, and allowlist save must not fail on flush warnings",
  );
  assert.doesNotMatch(
    firmwareSource,
    /return \(ret == ERRCODE_SUCC && flush_ret == ERRCODE_SUCC\) \? SLE_TEAM_OK : SLE_TEAM_ERR_UNSUPPORTED;/,
  );
  assert.match(serialCfgSource, /\[ValidateSet\("leader", "member", "status", "apply", "clear", "reboot"\)\]/);
  assert.match(serialCfgSource, /"apply" \{ return "cfg apply" \}/);
});

test("v4.4.48 accepts idempotent runtime role config without reopening SLE", () => {
  assert.match(firmwareSource, /static uint8_t team_serial_cfg_matches_runtime\(const sle_team_web_config_nv_t \*cfg\)/);
  assert.match(firmwareSource, /g_team_node\.cfg\.team_id != cfg->team_id/);
  assert.match(firmwareSource, /g_team_node\.cfg\.channel_hash != cfg->channel_hash/);
  assert.match(firmwareSource, /g_team_node\.cfg\.leader_id != leader_id/);
  assert.match(firmwareSource, /runtime already matches role=%u team=%u channel=%u leader_suffix=%04X/);
  assert.match(
    firmwareSource,
    /if \(g_team_rt\.role_configured != 0U\) \{[\s\S]+team_serial_cfg_matches_runtime\(cfg\)[\s\S]+return SLE_TEAM_OK;[\s\S]+return SLE_TEAM_ERR_UNSUPPORTED;/,
  );
});

test("v4.4.30 keeps member server advertising stable after an upstream connection", () => {
  const connectBlock =
    sleUartServerSource.match(
      /if \(conn_state == SLE_ACB_STATE_CONNECTED\) \{[\s\S]+?\} else if \(conn_state == SLE_ACB_STATE_DISCONNECTED\)/,
    )?.[0] ?? "";
  const disconnectBlock =
    sleUartServerSource.match(/else if \(conn_state == SLE_ACB_STATE_DISCONNECTED\) \{[\s\S]+?\n    \}/)?.[0] ??
    "";

  assert.match(connectBlock, /keep announce stable after connect/);
  assert.doesNotMatch(connectBlock, /sle_uart_server_adv_restart\(\)/);
  assert.match(disconnectBlock, /sle_uart_server_adv_restart\(\)/);
});

test("v4.4.30 keeps leader/client seek stopped after a SLE connection", () => {
  const connectBlock =
    sleUartClientSource.match(
      /if \(conn_state == SLE_ACB_STATE_CONNECTED\) \{[\s\S]+?\} else if \(conn_state == SLE_ACB_STATE_NONE\)/,
    )?.[0] ?? "";
  const disconnectBlock =
    sleUartClientSource.match(/else if \(conn_state == SLE_ACB_STATE_DISCONNECTED\) \{[\s\S]+?\n    \}/)?.[0] ?? "";

  assert.match(connectBlock, /keep seek stopped after connect/);
  assert.doesNotMatch(connectBlock, /sle_uart_start_scan\(\)/);
  assert.match(disconnectBlock, /sle_uart_start_scan\(\)/);
});

test("v4.4.30 sends client exchange-info at most once per SLE connection", () => {
  const pairCompleteBlock =
    sleUartClientSource.match(
      /void sle_uart_client_handle_pair_complete[\s\S]+?\r?\n}\r?\n\r?\nstatic void sle_uart_client_sample_exchange_info_cbk/,
    )?.[0] ?? "";

  assert.match(sleUartClientSource, /uint8_t exchange_requested;/);
  assert.match(sleUartClientSource, /static void sle_uart_client_exchange_once\(uint16_t conn_id, const char \*reason\)/);
  assert.match(sleUartClientSource, /if \(conn->exchange_requested != 0U\) \{[\s\S]+exchange info already requested/);
  assert.match(sleUartClientSource, /conn->exchange_requested = 1U;[\s\S]+ssapc_exchange_info_req\(0, conn_id, &info\);/);
  assert.match(sleUartClientSource, /sle_uart_client_exchange_once\(conn_id, "connect-paired"\)/);
  assert.match(pairCompleteBlock, /sle_uart_client_exchange_once\(conn_id, "pair-complete"\)/);
  assert.doesNotMatch(pairCompleteBlock, /ssapc_exchange_info_req/);
});

test("v4.4.30 client connect path skips forced pairing and exchanges in both pair states", () => {
  const connectBlock =
    sleUartClientSource.match(
      /if \(conn_state == SLE_ACB_STATE_CONNECTED\) \{[\s\S]+?\} else if \(conn_state == SLE_ACB_STATE_NONE\)/,
    )?.[0] ?? "";

  assert.match(connectBlock, /pair skip conn_id:%u pair_state:none/);
  assert.match(connectBlock, /if \(pair_state == SLE_PAIR_PAIRED\) \{[\s\S]+sle_uart_client_exchange_once\(conn_id, "connect-paired"\)/);
  assert.match(connectBlock, /sle_uart_client_exchange_once\(conn_id, "connect-unpaired"\)/);
  assert.doesNotMatch(connectBlock, /sle_pair_remote_device/);
  assert.doesNotMatch(connectBlock, /sle_uart_client_exchange_once\(conn_id, "connect"\)/);
  assert.match(sleUartClientSource, /pair complete conn_id:%d status:0x%x/);
  assert.match(sleUartServerSource, /ssaps_set_info ret:%x mtu:%u version:%u/);
});

test("v4.4.30 clears stale pair records and reports pair status names", () => {
  assert.match(
    sleUartClientSource,
    /static void sle_uart_client_remove_pairing\(const sle_addr_t \*addr, const char \*reason\)/,
  );
  assert.match(sleUartClientSource, /static uint8_t sle_uart_client_pair_should_reset\(errcode_t status\)/);
  assert.match(sleUartClientSource, /status == ERRCODE_SLE_PAIRING_REJECT/);
  assert.match(sleUartClientSource, /status == ERRCODE_SLE_AUTH_FAIL/);
  assert.match(sleUartClientSource, /status == ERRCODE_SLE_AUTH_PKEY_MISS/);
  assert.match(sleUartClientSource, /sle_uart_client_remove_pairing\(pair_addr,\s*"pair-complete-fail"\)/);
  assert.match(sleUartClientSource, /pair complete conn_id:%d status:0x%x\(%s\)/);
  assert.match(sleUartClientSource, /ERRCODE_SLE_AUTH_PKEY_MISS/);
});

test("v4.4.30 leader rescans when members are offline and keeps 30 logical members", () => {
  assert.match(nodeSource, /static int sle_team_sync_allowed_member_records\(sle_team_node_t \*node/);
  assert.match(nodeSource, /sle_team_alloc_offline_member_record\(node,\s*member_ids\[i\]\)/);
  assert.match(firmwareSource, /ret = sle_team_node_set_allowed_members\(&g_team_node/);
  assert.match(
    fs.readFileSync(path.join(repoRoot, "examples/team_node_regression_test.c"), "utf8"),
    /test_allowed_list_seeds_offline_logical_members/,
  );
  assert.match(firmwareSource, /static uint8_t team_leader_has_offline_members\(void\)/);
  assert.match(firmwareSource, /leader_has_offline_members = team_leader_has_offline_members\(\)/);
  assert.match(firmwareSource, /rescan_reason = "member_offline"/);
  assert.match(firmwareSource, /#define SLE_TEAM_MEMBER_RESCAN_INTERVAL_S 3U/);
  assert.match(
    fs.readFileSync(path.join(repoRoot, "include/sle_team_node.h"), "utf8"),
    /#define SLE_TEAM_MAX_LOGICAL_MEMBERS 30U/,
  );
  assert.match(
    fs.readFileSync(path.join(repoRoot, "examples/team_network_demo.c"), "utf8"),
    /assert\(leader\.cfg\.allowed_member_count == 30U\)/,
  );
  assert.match(
    fs.readFileSync(path.join(repoRoot, "scripts/simulate_python_1v20.sh"), "utf8"),
    /--members 30/,
  );
});

test("logical member rejoin reuses an offline slot instead of duplicating records", () => {
  assert.match(nodeSource, /static const sle_team_member_record_t \*sle_team_find_member_record_const/);
  assert.match(nodeSource, /if \(node->members\[i\]\.member_id == member_id\)/);
  assert.match(nodeSource, /if \(create != 0U\) \{\s*member->online = 1U;\s*\}/s);
  assert.match(
    fs.readFileSync(path.join(repoRoot, "examples/team_node_regression_test.c"), "utf8"),
    /test_offline_member_rejoin_reuses_logical_slot/,
  );
});

test("v4.4.45 keeps member online state ownership centralized", () => {
  const coreOnlineWrites = [...nodeSource.matchAll(/\b(?:member|free_slot)->online\s*=\s*[01]U/g)].map(
    (match) => match[0],
  );
  const firmwareOnlineWrites = [...firmwareSource.matchAll(/\bmember->online\s*=\s*[01]U/g)].map((match) => match[0]);

  assert.deepEqual(coreOnlineWrites, [
    "member->online = 1U",
    "free_slot->online = 1U",
    "member->online = 0U",
    "member->online = 0U",
  ]);
  assert.deepEqual(firmwareOnlineWrites, ["member->online = 0U"]);
  assert.match(nodeSource, /static void sle_team_prune_stale_members/);
  assert.match(firmwareSource, /static uint8_t team_leader_mark_member_offline/);
  assert.match(
    firmwareSource,
    /team_connection_state_changed_cbk[\s\S]+team_leader_mark_member_offline\(disconnected_member_id, "conn_disconnected"\)/,
  );
});

test("relay dual-role callback registration merges announce and seek handlers", () => {
  const serverAdvSource = fs.readFileSync(
    path.join(repoRoot, "xc/ws63_team_network/sle_uart_server/sle_uart_server_adv.c"),
    "utf8",
  );
  assert.match(serverAdvSource, /static sle_announce_seek_callbacks_t g_sle_uart_announce_seek_cbks = \{0\};/);
  assert.match(serverAdvSource, /errcode_t sle_uart_announce_seek_merge_cbks\(const sle_announce_seek_callbacks_t \*cbks\)/);
  assert.match(serverAdvSource, /ret = sle_uart_announce_seek_merge_cbks\(&seek_cbks\)/);
  assert.match(sleUartClientSource, /ret = sle_uart_announce_seek_merge_cbks\(&g_sle_uart_seek_cbk\)/);
});

test("webui role shortcuts and bulk deployment use unified cfg/config endpoints", () => {
  const serialApiClass = clientSource.match(/export class SerialTeamApi[\s\S]+?export function createTeamApi/)?.[0] ?? "";

  assert.match(mainSource, /data-form="bulk-config"/);
  assert.match(mainSource, /data-form="role-leader"/);
  assert.match(
    mainSource,
    /api\.configureRole\(\{\s*role: "leader",\s*teamId: readNumber\(form, "teamId"\) \?\? 1,\s*channel: readNumber\(form, "channel"\) \?\? 17,\s*\}\)/,
  );
  assert.match(mainSource, /data-action="config-read"/);
  assert.match(clientSource, /\/api\/config\/status/);
  assert.match(clientSource, /\/api\/config\/leader/);
  assert.match(clientSource, /\/api\/config\/member/);
  assert.match(clientSource, /configureDeviceNow\(\{ \.\.\.command, applyNow: true \}\)/);
  assert.doesNotMatch(clientSource, /configureDeviceNow\(\{ role: "leader", teamId: 1, channel: 17, applyNow: true \}\)/);
  assert.match(clientSource, /cfg status/);
  assert.match(clientSource, /return `cfg leader\$\{now\} \$\{command\.teamId\} \$\{command\.channel\}`;/);
  assert.match(
    clientSource,
    /return `cfg member\$\{now\} \$\{command\.leaderSuffix\} \$\{command\.teamId\} \$\{command\.channel\}`;/,
  );
  assert.match(clientSource, /cfg apply/);
  assert.match(clientSource, /cfg clear/);
  assert.match(serialApiClass, /private async configureDeviceNow\(command: DeviceConfigCommand\): Promise<void>/);
  assert.match(serialApiClass, /const result = await this\.configureDevice\(command\)/);
  assert.match(serialApiClass, /if \(!result\.ok\)/);
  assert.match(serialApiClass, /await this\.configureDeviceNow\(\{ \.\.\.command, applyNow: true \}\)/);
  assert.doesNotMatch(serialApiClass, /async configureRole[\s\S]+?await this\.runCli\(`?cfg (?:leader|member)/);
  assert.doesNotMatch(clientSource, /\/api\/role\?role=leader/);
  assert.doesNotMatch(clientSource, /runCli\("role leader"/);
  assert.doesNotMatch(clientSource, /runCli\(`role member/);
  assert.match(
    firmwareSource,
    /team_request_role_config\(SLE_TEAM_ROLE_LEADER,\s*g_team_rt\.route_id,\s*team_cfg_default_team\(\),\s*team_cfg_default_channel\(\)/,
  );
});

test("webui WebSerial uses one background reader so cfg-json lines are not dropped", () => {
  assert.match(clientSource, /async function readSerialLoop\(port: SerialPortLike\): Promise<void>/);
  assert.match(clientSource, /function startSerialReader\(port: SerialPortLike\): void/);
  assert.match(clientSource, /serialReaderTask = readSerialLoop\(port\)/);
  assert.match(clientSource, /waitForSerialLinesSince\(startSeq,\s*waitMs,\s*command\)/);
  assert.match(clientSource, /command === "cfg status"\) return hasCfgJson/);
  assert.doesNotMatch(clientSource, /Promise\.race\(\s*\[reader\.read\(\),\s*timeout\]\s*\)/);
  assert.doesNotMatch(clientSource, /ReadableStreamReadResult/);
});

test("webui WebSerial parses current member rows with operator labels and relay fields", () => {
  const memberParser = clientSource.match(/function cliMemberToNode[\s\S]+?\n}\n/)?.[0] ?? "";

  assert.match(memberParser, /member=\(\\d\+\).*?\\brole=/);
  assert.match(memberParser, /label=\(M\[0-9A-Fa-f\]\{4\}\)/);
  assert.match(memberParser, /mac=\(\[0-9A-Fa-f\]\{4\}\)\\s\+ready=\(\\d\+\)/);
  assert.match(memberParser, /relay=\(\\d\+\)/);
  assert.match(memberParser, /tier=\(\\d\+\)/);
  assert.match(memberParser, /max_down=\(\\d\+\)/);
  assert.match(memberParser, /relayAllowed: relayMatch \? Number\(relayMatch\[1\]\) !== 0 : undefined/);
  assert.match(memberParser, /macSuffix: macReady \? macMatch\?\.\[1\]\.toUpperCase\(\) : labelSuffix/);
});

test("webui config actions surface firmware and serial ret failures", () => {
  assert.match(mainSource, /function assertConfigResultOk\(result: DeviceConfigResult\): void/);
  assert.match(mainSource, /if \(!result\.ok\)/);
  assert.match(mainSource, /throw new Error\(`config \$\{result\.action\} failed ret=\$\{result\.ret\}`\)/);
  assert.match(mainSource, /const result = await api\.configureDevice\(command\);[\s\S]+?assertConfigResultOk\(result\);/);
  assert.match(mainSource, /const result = await api\.applyDeviceConfig\(\);[\s\S]+?assertConfigResultOk\(result\);/);
  assert.match(mainSource, /const result = await api\.clearDeviceConfig\(\);[\s\S]+?assertConfigResultOk\(result\);/);
});

test("webui bulk config form preserves valid channel zero", () => {
  assert.match(mainSource, /function configNumberValue\(.*fallback: number\): number/s);
  assert.match(mainSource, /configNumberValue\(config\?\.nvChannel,\s*config\?\.runtimeChannel,\s*17\)/);
  assert.doesNotMatch(mainSource, /config\?\.nvChannel\s*\|\|\s*config\?\.runtimeChannel\s*\|\|\s*17/);
});

test("serial bulk config helper does not toggle DTR or RTS by default", () => {
  assert.match(serialCfgSource, /\[switch\]\$UseControlLines/);
  assert.match(serialCfgSource, /Add-Type -AssemblyName System \| Out-Null/);
  assert.match(serialCfgSource, /\$serialPort = New-SerialPort/);
  assert.match(serialCfgSource, /\$sp\.DtrEnable = \$UseControlLines\.IsPresent/);
  assert.match(serialCfgSource, /\$sp\.RtsEnable = \$UseControlLines\.IsPresent/);
  assert.doesNotMatch(serialCfgSource, /\$port = New-SerialPort/);
  assert.doesNotMatch(serialCfgSource, /\$sp\.DtrEnable\s*=\s*\$true/);
  assert.doesNotMatch(serialCfgSource, /\$sp\.RtsEnable\s*=\s*\$true/);
});

test("cross-origin board actions expose redirect responses to hosted WebUI", () => {
  const redirectFunction = firmwareSource.match(/static void team_http_send_redirect[\s\S]+?\n}/)?.[0] ?? "";
  assert.match(redirectFunction, /Access-Control-Allow-Origin: \*/);
});

test("firmware softap defaults to v2-compatible mix+ax and includes wpa2 fallback", () => {
  const softapStart = firmwareSource.match(/static int team_wifi_ap_start[\s\S]+?\n}\n/)?.[0] ?? "";
  assert.match(softapStart, /ap_config\.security_type\s*=\s*SLE_TEAM_WIFI_SECURITY_COMPAT_MIX;/);
  assert.match(softapStart, /advance_config\.protocol_mode\s*=\s*SLE_TEAM_WIFI_PROTOCOL_COMPAT_AX;/);
  assert.match(softapStart, /ap_config\.security_type\s*=\s*WIFI_SEC_TYPE_WPA2PSK;/);
  assert.match(softapStart, /advance_fallback\.protocol_mode\s*=\s*WIFI_MODE_11B_G_N;/);
});

test("firmware event API exposes uptime seconds", () => {
  assert.match(webApiSource, /\\"time\\":\\"%lu\\"/);
  assert.match(webApiSource, /event->time_s/);
});

test("firmware nodes API exposes member location fields", () => {
  assert.match(webApiSource, /\\"latitudeE6\\":%ld/);
  assert.match(webApiSource, /\\"longitudeE6\\":%ld/);
});

test("firmware pairing page exposes phone GPS upload and auto-report bridge", () => {
  assert.match(firmwareSource, /GET \/api\/location/);
  assert.match(firmwareSource, /pairing-location-form/);
  assert.match(firmwareSource, /pairing-location-usegps/);
  assert.match(firmwareSource, /pairing-location-auto/);
  assert.match(firmwareSource, /navigator\.geolocation/);
  assert.match(firmwareSource, /watchPosition/);
  assert.match(firmwareSource, /clearWatch/);
});

test("v4.4 build keeps WS2812 build-enable path and buzzer io14 safe-off defaults", () => {
  assert.match(buildScriptSource, /set_kconfig_value\(s,\s*"CONFIG_SLE_TEAM_WS2812_ENABLE",\s*"y"\)/);
  assert.match(buildScriptSource, /unset_kconfig_bool\(s,\s*"CONFIG_SLE_TEAM_BUZZER_ENABLE"\)/);
  assert.match(buildScriptSource, /set_kconfig_value\(s,\s*"CONFIG_AT_UART",\s*"3"\)/);
  assert.match(buildScriptSource, /unset_kconfig_bool\(s,\s*"CONFIG_DYNAMIC_UART_ID_BINDDING"\)/);
  assert.match(firmwareSource, /#define CONFIG_SLE_TEAM_WS2812_ENABLE 0/);
  assert.match(firmwareSource, /#define CONFIG_SLE_TEAM_BUZZER_ENABLE 0/);
  assert.match(firmwareKconfigSource, /config SLE_TEAM_WS2812_ENABLE\s+bool "Enable v4 WS2812 RGB status LED"\s+default n/);
  assert.match(firmwareSource, /#define SLE_TEAM_BUZZER_FORCE_OFF_LEVEL GPIO_LEVEL_LOW/);
  assert.match(firmwareSource, /#define SLE_TEAM_BUZZER_FORCE_ON_LEVEL GPIO_LEVEL_HIGH/);
  assert.match(firmwareSource, /#define SLE_TEAM_BUZZER_TOGGLE_INTERVAL_MS 3000U/);
  assert.match(firmwareSource, /uapi_pin_set_pull\(pin,\s*PIN_PULL_TYPE_DOWN\)/);
  assert.match(firmwareSource, /void OHOS_SystemInit\(void\)/);
  assert.match(firmwareSource, /team_buzzer_force_pin_off\(\(uint8_t\)CONFIG_SLE_TEAM_BUZZER_PIN\)/);
  assert.match(firmwareSource, /#define CONFIG_SLE_TEAM_BUZZER_ACTIVE_HIGH 1/);
  assert.match(firmwareSource, /g_team_rt\.buzzer_active_high = \(uint8_t\)CONFIG_SLE_TEAM_BUZZER_ACTIVE_HIGH/);
  assert.match(firmwareSource, /team_buzzer_toggle_tick\(\)/);
  assert.match(firmwareSource, /\[diag\] buzzer io14 toggled level=%u interval_ms=%u/);
  assert.doesNotMatch(firmwareSource, /team_buzzer_beep\(1U/);
  assert.doesNotMatch(firmwareSource, /team_buzzer_beep\(2U/);
  assert.match(firmwareSource, /#define SLE_TEAM_WS2812_BOOT_R 0U/);
  assert.match(firmwareSource, /#define SLE_TEAM_WS2812_BOOT_G 24U/);
  assert.match(firmwareSource, /#define SLE_TEAM_WS2812_BOOT_B 64U/);
  assert.match(firmwareSource, /team_ws2812_set_rgb\(SLE_TEAM_WS2812_BOOT_R,\s*SLE_TEAM_WS2812_BOOT_G,\s*SLE_TEAM_WS2812_BOOT_B\)/);
});

test("v4.4 display tuple stays aligned across Kconfig, fallback defines, and build script", () => {
  for (const [key, value] of [
    ["X_OFFSET", "40"],
    ["Y_OFFSET", "53"],
    ["WIDTH", "240"],
    ["HEIGHT", "135"],
  ]) {
    assert.match(firmwareSource, new RegExp(`#define CONFIG_SLE_TEAM_ST7789_${key} ${value}`));
    assert.match(firmwareKconfigSource, new RegExp(`config SLE_TEAM_ST7789_${key}[\\s\\S]+?default ${value}`));
    assert.match(buildScriptSource, new RegExp(`CONFIG_SLE_TEAM_ST7789_${key}", "${value}"`));
  }
  assert.match(buildScriptSource, /set_kconfig_value\(s,\s*"CONFIG_SLE_TEAM_DISPLAY_USE_LVGL",\s*"y"\)/);
  assert.match(buildScriptSource, /set_kconfig_value\(s,\s*"CONFIG_SLE_TEAM_LVGL_DRAW_BUF_LINES",\s*"8"\)/);
  assert.match(displaySource, /#define ST7789_MADCTL_DEFAULT 0x60U/);
});

test("LVGL dependency is reproducible from GitHub checkout", () => {
  assert.match(gitmodulesSource, /\[submodule "xc\/ws63_team_network\/third_party\/lvgl"\]/);
  assert.match(gitmodulesSource, /path = xc\/ws63_team_network\/third_party\/lvgl/);
  assert.match(gitmodulesSource, /url = https:\/\/github\.com\/lvgl\/lvgl\.git/);
  assert.match(cmakeSource, /third_party\/lvgl/);
  assert.match(cmakeSource, /message\(FATAL_ERROR "LVGL source not found/);
  assert.match(lvglPatchSource, /lv_draw_sw_rect\.c/);
  assert.match(buildScriptSource, /LVGL_PATCH/);
  assert.match(buildScriptSource, /LVGL patch already present in source/);
  assert.match(buildScriptSource, /git apply --unidiff-zero --check "\$LVGL_PATCH"/);
  assert.match(buildScriptSource, /git apply --unidiff-zero --reverse --check "\$LVGL_PATCH"/);
});

test("webui keeps wifi send form gated and location button non-submit", () => {
  assert.match(mainSource, /if \(state\.connection\.mode === "wifi"\)/);
  assert.match(mainSource, /WiFi 模式暂不提供 \/api\/send/);
  assert.doesNotMatch(mainSource, /renderPhoneLocationPanel/);
  assert.doesNotMatch(mainSource, /data-action="send-phone-location"/);
});

test("webui exposes serial-mode guidance for unsupported factory reset", () => {
  assert.match(mainSource, /state\.connection\.mode === "serial"/);
  assert.match(mainSource, /串口模式暂不支持 factory reset/);
  assert.match(mainSource, /data-action="factory-reset"[^`]*factoryResetDisabled \? "disabled" : ""/);
});

test("route metrics payload drops routeHintLastActivityS to save firmware memory/json size", () => {
  assert.doesNotMatch(webApiSource, /routeHintLastActivityS/);
  assert.doesNotMatch(
    fs.readFileSync(path.join(repoRoot, "webui/src/protocol/types.ts"), "utf8"),
    /routeHintLastActivityS/,
  );
});

test("firmware trims conn track addr while preserving pending lookup addr", () => {
  const connTrackStruct = firmwareSource.match(/typedef struct \{[\s\S]*?\n\} team_conn_track_t;/)?.[0] ?? "";
  const pendingStruct = firmwareSource.match(/typedef struct \{[\s\S]*?\n\} team_pending_conn_t;/)?.[0] ?? "";
  assert.doesNotMatch(connTrackStruct, /\bsle_addr_t addr;/);
  assert.match(pendingStruct, /\bsle_addr_t addr;/);
});

test("relay rebalance offline revoke is explicit no-notify path and active paths notify", () => {
  assert.match(
    firmwareSource,
    /team_leader_set_member_relay_allowed\(member,\s*0U,\s*"offline",\s*0U\)/,
  );
  assert.match(
    firmwareSource,
    /team_leader_set_member_relay_allowed\(member,\s*0U,\s*"stale",\s*1U\)/,
  );
  assert.match(
    firmwareSource,
    /team_leader_set_member_relay_allowed\(victim,\s*0U,\s*"auto-demote",\s*1U\)/,
  );
  assert.match(
    firmwareSource,
    /team_leader_set_member_relay_allowed\(candidate,\s*1U,\s*"auto-promote",\s*1U\)/,
  );
  assert.match(
    firmwareSource,
    /static void team_leader_rebalance_relays\(uint8_t force_now\)/,
  );
  assert.match(
    firmwareSource,
    /if \(force_now == 0U[\s\S]*team_interval_not_reached\(/,
  );
  assert.match(
    firmwareSource,
    /team_leader_rebalance_relays\(1U\)/,
  );
});

test("relay swap requires 8 dB hysteresis for 30 seconds before exchanging roles", () => {
  assert.match(firmwareSource, /#define SLE_TEAM_RELAY_SWAP_RSSI_HYST_DBM 8/);
  assert.match(firmwareSource, /#define SLE_TEAM_RELAY_SWAP_STABLE_S 30U/);
  assert.match(
    firmwareSource,
    /rssi_delta = \(int16_t\)candidate->last_rssi_dbm - \(int16_t\)victim->last_rssi_dbm/,
  );
  assert.match(
    firmwareSource,
    /rssi_delta < \(int16_t\)SLE_TEAM_RELAY_SWAP_RSSI_HYST_DBM/,
  );
  assert.match(
    firmwareSource,
    /stable_s < SLE_TEAM_RELAY_SWAP_STABLE_S/,
  );
  assert.match(
    firmwareSource,
    /team_leader_set_member_relay_allowed\(candidate,\s*1U,\s*"swap-promote",\s*1U\)/,
  );
  assert.match(
    firmwareSource,
    /team_leader_set_member_relay_allowed\(victim,\s*0U,\s*"swap-demote",\s*1U\)/,
  );
  assert.match(
    firmwareSource,
    /pending_count == 0U && known_count == online_count[\s\S]+team_leader_relay_swap_tick\(now_s,\s*timeout_s,\s*1U\)/,
  );
});

test("firmware i32 query parser avoids 32-bit signed overflow at bounds", () => {
  const i32Parser = firmwareSource.match(/static int team_http_query_i32[\s\S]+?\n}\n/)?.[0] ?? "";
  assert.match(i32Parser, /int64_t signed_value;/);
  assert.match(i32Parser, /abs_value > 2147483648UL/);
  assert.match(i32Parser, /abs_value > 2147483647UL/);
  assert.match(i32Parser, /signed_value = negative != 0U \? -\(int64_t\)abs_value : \(int64_t\)abs_value;/);
});

test("hello ack path keeps relay_enabled sync when config is cached before ack", () => {
  const nodeSource = fs.readFileSync(path.join(repoRoot, "src/sle_team_node.c"), "utf8");
  assert.match(nodeSource, /CONFIG may arrive before ACK/);
  assert.match(nodeSource, /node->cfg\.relay_enabled = node->cfg\.relay_allowed != 0U \? 1U : 0U;/);
});

test("upstream disconnect uses link-lost recovery instead of manual leave", () => {
  const firmware = firmwareSource;
  assert.match(firmware, /switch_ret = sle_team_node_try_parent_switch\(&g_team_node\)/);
  assert.match(firmware, /recover_ret = sle_team_node_member_link_lost\(&g_team_node\)/);
  assert.match(firmware, /\[team\] upstream link lost switch_ret=%d recover_ret=%d/);
  assert.doesNotMatch(firmware, /switch_ret[\s\S]{0,240}sle_team_node_member_leave\(&g_team_node\)/);
});

test("pair approve rolls back newly-added allowlist entry when member slot allocation fails", () => {
  const nodeSource = fs.readFileSync(path.join(repoRoot, "src/sle_team_node.c"), "utf8");
  assert.match(nodeSource, /had_allowed_before = 0U/);
  assert.match(nodeSource, /if \(had_allowed_before == 0U\) \{\s*\(void\)sle_team_node_remove_allowed_member\(node,\s*member_id\);/s);
});

test("pairing stop preserves members already online before the pairing window", () => {
  const nodeSource = fs.readFileSync(path.join(repoRoot, "src/sle_team_node.c"), "utf8");
  const pairingStop = nodeSource.match(/int sle_team_node_pairing_stop[\s\S]+?\n}\n/)?.[0] ?? "";

  assert.match(pairingStop, /Preserve members that were already connected before the pairing window/);
  assert.match(pairingStop, /known_ids\[known_count\+\+\] = node->members\[i\]\.member_id;/);
  assert.match(pairingStop, /sle_team_node_add_allowed_member\(node,\s*known_ids\[i\]\)/);
  assert.ok(
    pairingStop.indexOf("sle_team_node_add_allowed_member(node, known_ids[i])") <
      pairingStop.indexOf("sle_team_node_pairing_approve_with_relay(node, pending_ids[i], 0U)"),
  );
});

test("firmware numeric query parsing requires a clean terminator", () => {
  assert.match(firmwareSource, /\*p != '\\0' && \*p != '&'/);
});

test("config body base size derives from struct layout instead of a hardcoded byte count", () => {
  const nodeSource = fs.readFileSync(path.join(repoRoot, "src/sle_team_node.c"), "utf8");
  assert.match(
    nodeSource,
    /#define SLE_TEAM_CONFIG_BODY_BASE_SIZE offsetof\(sle_team_config_body_t,\s*relay_allowed\)/,
  );
});

test("route update parser and alert parser use memcpy instead of pointer casts", () => {
  const nodeSource = fs.readFileSync(path.join(repoRoot, "src/sle_team_node.c"), "utf8");

  assert.match(nodeSource, /memcpy\(&route_update,\s*app->body,\s*sizeof\(route_update\)\)/);
  assert.doesNotMatch(nodeSource, /\(const sle_team_route_update_body_t \*\)app->body/);
  assert.match(nodeSource, /memcpy\(&alert,\s*app->body,\s*sizeof\(alert\)\)/);
  assert.doesNotMatch(nodeSource, /\(const sle_team_alert_body_t \*\)app->body/);
});

test("route update relay enable sync uses explicit relay-grant flag", () => {
  const packetHeader = fs.readFileSync(path.join(repoRoot, "include/sle_team_packet.h"), "utf8");
  const nodeSource = fs.readFileSync(path.join(repoRoot, "src/sle_team_node.c"), "utf8");

  assert.match(packetHeader, /#define SLE_TEAM_ROUTE_UPDATE_FLAG_RELAY_GRANT 0x01U/);
  assert.match(nodeSource, /route_update\.reserved & SLE_TEAM_ROUTE_UPDATE_FLAG_RELAY_GRANT/);
  assert.match(nodeSource, /if \(node->cfg\.role == SLE_TEAM_ROLE_LEADER && parent_state != 0U\) \{\s*route_update\.reserved \|= SLE_TEAM_ROUTE_UPDATE_FLAG_RELAY_GRANT;/s);
});

test("relay discovery-only forwards join control packets and blocks business broadcasts", () => {
  const nodeSource = fs.readFileSync(path.join(repoRoot, "src/sle_team_node.c"), "utf8");

  assert.match(nodeSource, /sle_team_discovery_only_allows\(uint8_t app_msg_type\)/);
  assert.match(nodeSource, /app_msg_type == SLE_TEAM_APP_HELLO/);
  assert.match(nodeSource, /app_msg_type == SLE_TEAM_APP_ROUTE_UPDATE/);
  assert.match(nodeSource, /app_msg_type == SLE_TEAM_APP_CONFIG/);
  assert.match(nodeSource, /app_msg_type == SLE_TEAM_APP_ACK/);
  assert.match(nodeSource, /relay_discovery_only != 0U[\s\S]*sle_team_discovery_only_allows\(app->app_msg_type\) == 0U/);
  assert.match(nodeSource, /relay_discovery_only != 0U[\s\S]*app_packet\.dst_id == SLE_TEAM_BROADCAST_ID[\s\S]*sle_team_discovery_only_allows\(app_packet\.app_msg_type\) == 0U/);
});

test("relay tier bucket count is a named constant", () => {
  const nodeSource = fs.readFileSync(path.join(repoRoot, "src/sle_team_node.c"), "utf8");
  assert.match(nodeSource, /#define SLE_TEAM_MAX_RELAY_TIERS 3U/);
  assert.match(nodeSource, /member_id == leader_id/);
  assert.match(nodeSource, /% SLE_TEAM_MAX_RELAY_TIERS\)/);
});

test("webui keeps dev:https alias mapped to dev command", () => {
  assert.equal(typeof webPackage.scripts["dev:https"], "string");
  assert.match(webPackage.scripts["dev:https"], /--host 0\.0\.0\.0/);
  assert.equal(webPackage.scripts["dev:https"], webPackage.scripts.dev);
});

test("vite config enables basic ssl in both dev and preview", () => {
  assert.match(viteConfigSource, /import basicSsl from "@vitejs\/plugin-basic-ssl"/);
  assert.match(viteConfigSource, /plugins:\s*\[\s*basicSsl\(/);
  assert.match(viteConfigSource, /server:\s*\{[\s\S]*https:\s*true/);
  assert.match(viteConfigSource, /preview:\s*\{[\s\S]*https:\s*true/);
});

test("webui readme local dev URL matches the https dev server", () => {
  assert.match(webReadmeSource, /https:\/\/localhost:5173\//);
  assert.doesNotMatch(webReadmeSource, /http:\/\/localhost:5173\//);
});

test("webui readme includes board location bridge route", () => {
  assert.match(webReadmeSource, /GET \/api\/location\?/);
  assert.doesNotMatch(webReadmeSource, /手机局域网定位建议使用 HTTPS/);
});
