import assert from "node:assert/strict";
import fs from "node:fs";
import path from "node:path";
import test from "node:test";

/*
 * Structural contract tests:
 * Some assertions intentionally match source snippets to lock behavior/shape.
 * If formatting/refactor changes these snippets, update the regex accordingly.
 */
const repoRoot = path.resolve(new URL("../..", import.meta.url).pathname);
const contract = JSON.parse(fs.readFileSync(path.join(repoRoot, "webui/shared/ws63-api.json"), "utf8"));
const webPackage = JSON.parse(fs.readFileSync(path.join(repoRoot, "webui/package.json"), "utf8"));
const viteConfigSource = fs.readFileSync(path.join(repoRoot, "webui/vite.config.ts"), "utf8");
const webReadmeSource = fs.readFileSync(path.join(repoRoot, "webui/README.md"), "utf8");
const firmwareSource = fs.readFileSync(
  path.join(repoRoot, "xc/ws63_team_network/src/ws63_team_network_app.c"),
  "utf8",
);
const webApiSource = fs.readFileSync(path.join(repoRoot, "src/sle_team_web_api.c"), "utf8");
const mainSource = fs.readFileSync(path.join(repoRoot, "webui/src/main.ts"), "utf8");

test("WS63 API contract lists the current board HTTP routes", () => {
  assert.deepEqual(
    contract.routes.map((route) => route.path),
    [
      "/api/status",
      "/api/nodes",
      "/api/events",
      "/api/pending",
      "/api/location",
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

test("v4.1 build keeps WS2812 startup marker and toggles buzzer io14 every 3s", () => {
  const buildScriptSource = fs.readFileSync(path.join(repoRoot, "scripts/ws63_build_v4_ubuntu.sh"), "utf8");
  assert.match(buildScriptSource, /set_kconfig_value\(s,\s*"CONFIG_SLE_TEAM_WS2812_ENABLE",\s*"y"\)/);
  assert.match(buildScriptSource, /unset_kconfig_bool\(s,\s*"CONFIG_SLE_TEAM_BUZZER_ENABLE"\)/);
  assert.match(buildScriptSource, /set_kconfig_value\(s,\s*"CONFIG_AT_UART",\s*"3"\)/);
  assert.match(buildScriptSource, /unset_kconfig_bool\(s,\s*"CONFIG_DYNAMIC_UART_ID_BINDDING"\)/);
  assert.match(firmwareSource, /#define CONFIG_SLE_TEAM_WS2812_ENABLE 1/);
  assert.match(firmwareSource, /#define CONFIG_SLE_TEAM_BUZZER_ENABLE 0/);
  assert.match(firmwareSource, /#define SLE_TEAM_BUZZER_FORCE_OFF_LEVEL GPIO_LEVEL_LOW/);
  assert.match(firmwareSource, /#define SLE_TEAM_BUZZER_FORCE_ON_LEVEL GPIO_LEVEL_HIGH/);
  assert.match(firmwareSource, /#define SLE_TEAM_BUZZER_TOGGLE_INTERVAL_MS 3000U/);
  assert.match(firmwareSource, /uapi_pin_set_pull\(pin,\s*PIN_PULL_TYPE_DOWN\)/);
  assert.match(firmwareSource, /void OHOS_SystemInit\(void\)/);
  assert.match(firmwareSource, /team_buzzer_force_pin_off\(\(uint8_t\)CONFIG_SLE_TEAM_BUZZER_PIN\)/);
  assert.match(firmwareSource, /g_team_rt\.buzzer_active_high = 1U/);
  assert.match(firmwareSource, /team_buzzer_toggle_tick\(\)/);
  assert.match(firmwareSource, /\[diag\] buzzer io14 toggled level=%u interval_ms=%u/);
  assert.doesNotMatch(firmwareSource, /team_buzzer_beep\(1U/);
  assert.doesNotMatch(firmwareSource, /team_buzzer_beep\(2U/);
  assert.match(firmwareSource, /#define SLE_TEAM_WS2812_BOOT_R 0U/);
  assert.match(firmwareSource, /#define SLE_TEAM_WS2812_BOOT_G 24U/);
  assert.match(firmwareSource, /#define SLE_TEAM_WS2812_BOOT_B 64U/);
  assert.match(firmwareSource, /team_ws2812_set_rgb\(SLE_TEAM_WS2812_BOOT_R,\s*SLE_TEAM_WS2812_BOOT_G,\s*SLE_TEAM_WS2812_BOOT_B\)/);
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

test("firmware i32 query parser avoids 32-bit signed overflow at bounds", () => {
  const i32Parser = firmwareSource.match(/static int team_http_query_i32[\s\S]+?\n}\n/)?.[0] ?? "";
  assert.match(i32Parser, /int64_t signed_value;/);
  assert.match(i32Parser, /abs_value > 2147483648UL/);
  assert.match(i32Parser, /abs_value > 2147483647UL/);
  assert.match(i32Parser, /signed_value = negative != 0U \? -\(int64_t\)abs_value : \(int64_t\)abs_value;/);
});

test("hello ack path keeps relay_enabled sync when config is cached before ack", () => {
  const ackHandler = fs.readFileSync(path.join(repoRoot, "src/sle_team_node.c"), "utf8").match(
    /static int sle_team_handle_ack[\s\S]+?\n}\n\nstatic int sle_team_handle_config/,
  )?.[0] ?? "";
  assert.match(ackHandler, /node->cfg\.relay_enabled = node->cfg\.relay_allowed != 0U \? 1U : 0U;/);
});

test("upstream disconnect prefers lightweight parent switch before full leave", () => {
  const firmware = firmwareSource;
  assert.match(firmware, /switch_ret = sle_team_node_try_parent_switch\(&g_team_node\)/);
  assert.match(
    firmware,
    /if \(switch_ret != SLE_TEAM_OK && switch_ret != SLE_TEAM_ERR_UNSUPPORTED\) \{\s*\(void\)sle_team_node_member_leave\(&g_team_node\);/s,
  );
});

test("pair approve rolls back newly-added allowlist entry when member slot allocation fails", () => {
  const nodeSource = fs.readFileSync(path.join(repoRoot, "src/sle_team_node.c"), "utf8");
  const approveFn = nodeSource.match(/int sle_team_node_pairing_approve_with_relay[\s\S]+?\n}\n\nint sle_team_node_member_select_leader/)?.[0] ?? "";
  assert.match(approveFn, /had_allowed_before = 0U/);
  assert.match(approveFn, /sle_team_node_remove_allowed_member\(node,\s*member_id\)/);
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
  const routeHandler = nodeSource.match(/static int sle_team_handle_route_update[\s\S]+?\n}\n\nint sle_team_node_init/)?.[0] ?? "";
  const alertHandler = nodeSource.match(/static int sle_team_handle_alert[\s\S]+?\n}\n\nstatic int sle_team_handle_route_update/)?.[0] ?? "";

  assert.match(routeHandler, /memcpy\(&route_update,\s*app->body,\s*sizeof\(route_update\)\)/);
  assert.doesNotMatch(routeHandler, /\(const sle_team_route_update_body_t \*\)app->body/);
  assert.match(alertHandler, /memcpy\(&alert,\s*app->body,\s*sizeof\(alert\)\)/);
  assert.doesNotMatch(alertHandler, /\(const sle_team_alert_body_t \*\)app->body/);
});

test("route update relay enable sync uses explicit relay-grant flag", () => {
  const packetHeader = fs.readFileSync(path.join(repoRoot, "include/sle_team_packet.h"), "utf8");
  const nodeSource = fs.readFileSync(path.join(repoRoot, "src/sle_team_node.c"), "utf8");
  const routeSendFn = nodeSource.match(/int sle_team_node_send_route_update[\s\S]+?\n}\n\nconst sle_team_member_record_t \*sle_team_node_find_member/)?.[0] ?? "";

  assert.match(packetHeader, /#define SLE_TEAM_ROUTE_UPDATE_FLAG_RELAY_GRANT 0x01U/);
  assert.match(nodeSource, /route_update\.reserved & SLE_TEAM_ROUTE_UPDATE_FLAG_RELAY_GRANT/);
  assert.match(routeSendFn, /if \(node->cfg\.role == SLE_TEAM_ROLE_LEADER && parent_state != 0U\) \{\s*route_update\.reserved \|= SLE_TEAM_ROUTE_UPDATE_FLAG_RELAY_GRANT;/s);
});

test("relay discovery-only nodes ignore non-discovery local broadcasts", () => {
  const nodeSource = fs.readFileSync(path.join(repoRoot, "src/sle_team_node.c"), "utf8");
  const onPacketFn = nodeSource.match(/int sle_team_node_on_packet[\s\S]+?\n}\n\nint sle_team_node_send_hello/)?.[0] ?? "";

  assert.match(
    onPacketFn,
    /relay_discovery_only != 0U[\s\S]*app_packet\.dst_id == SLE_TEAM_BROADCAST_ID[\s\S]*app_packet\.app_msg_type != SLE_TEAM_APP_HELLO[\s\S]*app_packet\.app_msg_type != SLE_TEAM_APP_ROUTE_UPDATE/,
  );
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
