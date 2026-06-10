#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'USAGE'
Usage:
  scripts/build/ws63_build_v4_ubuntu.sh unified

Builds the v4 WS63 team firmware on a LAN Ubuntu build machine.

Environment is the same as scripts/build/ws63_build_team_ubuntu.sh:
  UBUNTU_HOST=192.168.1.50
  UBUNTU_PORT=22
  UBUNTU_USER=builder
  UBUNTU_PASS=builder
  UBUNTU_SDK=/home/builder/workspace/bearpi-pico_h3863_fresh
  OUT_ROOT=/path/to/output_from_vm
  BUILD_JOBS=4
USAGE
}

role="${1:-unified}"
case "$role" in
  unified|leader|member)
    self_id=1
    out_dir="team_network_v4_unified_runtime_role"
    out_name="ws63-liteos-app_v4_unified_all.fwpkg"
    ;;
  -h|--help|"")
    usage
    exit 0
    ;;
  *)
    echo "Unknown role: $role" >&2
    usage >&2
    exit 2
    ;;
esac

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
UBUNTU_HOST="${UBUNTU_HOST:-}"
UBUNTU_PORT="${UBUNTU_PORT:-22}"
UBUNTU_USER="${UBUNTU_USER:-builder}"
UBUNTU_PASS="${UBUNTU_PASS:-}"
UBUNTU_SDK="${UBUNTU_SDK:-/home/builder/workspace/bearpi-pico_h3863_fresh}"
OUT_ROOT="${OUT_ROOT:-$ROOT_DIR/output_from_vm}"
BUILD_JOBS="${BUILD_JOBS:-4}"

next_archive_path() {
  local latest="$1"
  local version="$2"
  local base="${latest%.fwpkg}_${version}"
  local candidate="${base}.fwpkg"
  local index=1
  while [[ -e "$candidate" ]]; do
    candidate="${base}.${index}.fwpkg"
    index=$((index + 1))
  done
  printf '%s\n' "$candidate"
}

if [[ -z "$UBUNTU_HOST" ]]; then
  echo "UBUNTU_HOST is required, for example: UBUNTU_HOST=192.168.1.50 $0 $role" >&2
  exit 2
fi

CONFIG_PATH="$UBUNTU_SDK/build/config/target_config/ws63/menuconfig/acore/ws63_liteos_app.config"
REMOTE_PKG="$UBUNTU_SDK/output/ws63/fwpkg/ws63-liteos-app/ws63-liteos-app_all.fwpkg"
REMOTE_PROTO="$UBUNTU_SDK/third_party/sle_mesh"
REMOTE_APP="$UBUNTU_SDK/application/samples/products/sle_team_network"
LOCAL_OUT="$OUT_ROOT/$out_dir/$out_name"
ARCHIVE_OUT="$(next_archive_path "$LOCAL_OUT" "v4.4.137")"
LVGL_PATCH="$REMOTE_APP/third_party/lvgl-patches/lv8.3.11-ws63-c89-rect.patch"

ssh_opts=(
  -o StrictHostKeyChecking=no
  -o UserKnownHostsFile=/dev/null
  -o LogLevel=ERROR
)

if [[ -n "$UBUNTU_PASS" ]]; then
  ssh_base=(sshpass -p "$UBUNTU_PASS" ssh "${ssh_opts[@]}")
  rsync_ssh=(sshpass -p "$UBUNTU_PASS" ssh "${ssh_opts[@]}" -p "$UBUNTU_PORT")
else
  ssh_base=(ssh "${ssh_opts[@]}")
  rsync_ssh=(ssh "${ssh_opts[@]}" -p "$UBUNTU_PORT")
fi

ssh_cmd=("${ssh_base[@]}" -p "$UBUNTU_PORT" "$UBUNTU_USER@$UBUNTU_HOST")

echo "WS63 Ubuntu build"
echo "profile:    v4.4.137 unified runtime role (v3.2 schematic pinmap + ADC battery + TP4054 CHRG + RGB blink states)"
echo "fallback id:$self_id"
echo "host:       $UBUNTU_USER@$UBUNTU_HOST:$UBUNTU_PORT"
echo "sdk:        $UBUNTU_SDK"
echo "archive:    $ARCHIVE_OUT"
echo "latest:     $LOCAL_OUT"
echo

"${ssh_cmd[@]}" "test -f '$CONFIG_PATH' && mkdir -p '$REMOTE_PROTO' '$REMOTE_APP'"

rsync -az --delete \
  --exclude '.git' \
  --exclude 'build' \
  --exclude 'dist' \
  --exclude 'node_modules' \
  -e "${rsync_ssh[*]}" \
  "$ROOT_DIR/include/" "$UBUNTU_USER@$UBUNTU_HOST:$REMOTE_PROTO/include/"

rsync -az --delete \
  --exclude '.git' \
  --exclude 'build' \
  --exclude 'dist' \
  --exclude 'node_modules' \
  -e "${rsync_ssh[*]}" \
  "$ROOT_DIR/src/" "$UBUNTU_USER@$UBUNTU_HOST:$REMOTE_PROTO/src/"

rsync -az --delete \
  --exclude '.git' \
  --exclude 'build' \
  --exclude 'dist' \
  --exclude 'node_modules' \
  -e "${rsync_ssh[*]}" \
  "$ROOT_DIR/xc/ws63_team_network/" "$UBUNTU_USER@$UBUNTU_HOST:$REMOTE_APP/"

"${ssh_cmd[@]}" "bash -s -- '$LVGL_PATCH' '$REMOTE_APP'" <<'SH'
LVGL_PATCH="$1"
REMOTE_APP="$2"
if [ -f "$LVGL_PATCH" ]; then
  cd "$REMOTE_APP/third_party/lvgl"
  if grep -q "lv_area_t center_coords;" src/draw/sw/lv_draw_sw_rect.c &&
    grep -q "bool mask_any_center = false;" src/draw/sw/lv_draw_sw_rect.c; then
    echo "LVGL patch already present in source"
  elif git apply --unidiff-zero --check "$LVGL_PATCH"; then
    git apply --unidiff-zero "$LVGL_PATCH"
  elif git apply --unidiff-zero --reverse --check "$LVGL_PATCH"; then
    echo "LVGL patch already applied"
  else
    echo "LVGL patch check failed: $LVGL_PATCH" >&2
    exit 1
  fi
fi
SH

"${ssh_cmd[@]}" "python3 - '$CONFIG_PATH' '$self_id'" <<'PY'
from pathlib import Path
import sys

path = Path(sys.argv[1])
self_id = sys.argv[2]
s = path.read_text()

def set_kconfig_value(text, key, value):
    lines = text.splitlines()
    found = False
    out = []
    for line in lines:
        if line.startswith(key + "=") or line.startswith(f"# {key} is not set"):
            if not found:
                out.append(f"{key}={value}")
                found = True
            continue
        out.append(line)
    if not found:
        out.append(f"{key}={value}")
    return "\n".join(out) + "\n"

def unset_kconfig_bool(text, key):
    lines = text.splitlines()
    found = False
    out = []
    for line in lines:
        if line.startswith(key + "=") or line.startswith(f"# {key} is not set"):
            if not found:
                out.append(f"# {key} is not set")
                found = True
            continue
        out.append(line)
    if not found:
        out.append(f"# {key} is not set")
    return "\n".join(out) + "\n"

s = set_kconfig_value(s, "CONFIG_SLE_TEAM_SELF_ID", self_id)
s = set_kconfig_value(s, "CONFIG_SAMPLE_ENABLE", "y")
s = set_kconfig_value(s, "CONFIG_SAMPLE_SUPPORT_SLE_TEAM_NETWORK", "y")
s = unset_kconfig_bool(s, "CONFIG_SAMPLE_SUPPORT_SLE_UART")
s = unset_kconfig_bool(s, "CONFIG_SAMPLE_SUPPORT_SLE_UART_SERVER")
s = unset_kconfig_bool(s, "CONFIG_SAMPLE_SUPPORT_SLE_UART_CLIENT")
s = unset_kconfig_bool(s, "CONFIG_SAMPLE_SUPPORT_SLE_UART_1_VS_8")
s = unset_kconfig_bool(s, "CONFIG_SAMPLE_SUPPORT_SLE_UART_SERVER_1_VS_8")
s = unset_kconfig_bool(s, "CONFIG_SAMPLE_SUPPORT_SLE_UART_CLIENT_1_VS_8")
s = unset_kconfig_bool(s, "CONFIG_SAMPLE_SUPPORT_BLE_UART")
s = unset_kconfig_bool(s, "CONFIG_SAMPLE_SUPPORT_SLE_GETAWAY")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_LEADER_ID", "1")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_UART_BUS", "0")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_UART_TXD_PIN", "21")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_UART_RXD_PIN", "22")
s = set_kconfig_value(s, "CONFIG_AT_UART", "3")
s = unset_kconfig_bool(s, "CONFIG_DYNAMIC_UART_ID_BINDDING")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_LED_PIN", "255")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_WS2812_ENABLE", "y")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_WS2812_PIN", "0")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_BUZZER_ENABLE", "y")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_BUZZER_PIN", "14")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_BUZZER_ACTIVE_HIGH", "y")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_BUZZER_MUTED", "y")
s = unset_kconfig_bool(s, "CONFIG_SLE_TEAM_BUZZER_AUTO_TOGGLE")
s = unset_kconfig_bool(s, "CONFIG_SLE_TEAM_GPS_ENABLE")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_GPS_UART_BUS", "1")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_GPS_UART_TXD_PIN", "17")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_GPS_UART_RXD_PIN", "18")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_ADC_ENABLE", "y")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_ADC_CTRL_PIN", "5")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_ADC_VBAT_PIN", "12")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_ADC_VBAT_CHANNEL", "5")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_ADC_CTRL_ACTIVE_HIGH", "y")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_ADC_SAMPLE_SETTLE_MS", "50")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_ADC_SAMPLE_INTERVAL_S", "30")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_CHRG_ENABLE", "y")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_CHRG_PIN", "2")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_CHRG_ACTIVE_LOW", "y")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_CHRG_EXTERNAL_PULLUP", "y")
s = set_kconfig_value(s, "CONFIG_ADC_SUPPORT_AUTO_SCAN", "y")
s = set_kconfig_value(s, "CONFIG_ADC_USING_V154", "y")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_ST7789_ENABLE", "y")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_ST7789_SPI_BUS", "0")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_ST7789_SCLK_PIN", "7")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_ST7789_MOSI_PIN", "9")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_ST7789_CS_PIN", "8")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_ST7789_CS_ALWAYS_LOW", "y")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_ST7789_DC_PIN", "13")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_ST7789_RESET_PIN", "10")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_ST7789_X_OFFSET", "40")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_ST7789_Y_OFFSET", "53")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_ST7789_WIDTH", "240")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_ST7789_HEIGHT", "135")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_DISPLAY_USE_LVGL", "y")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_LVGL_DRAW_BUF_LINES", "8")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_HEARTBEAT_INTERVAL_S", "1")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_HEARTBEAT_TIMEOUT_S", "3")
s = set_kconfig_value(s, "CONFIG_SPI_SUPPORT_MASTER", "y")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_WIFI_AP_ENABLE", "y")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_WIFI_AP_AUTO_START", "y")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_WIFI_AP_SSID", '"SLE-TEAM-V4"')
s = set_kconfig_value(s, "CONFIG_SUPPORT_SLE_PERIPHERAL", "y")
s = set_kconfig_value(s, "CONFIG_SUPPORT_SLE_CENTRAL", "y")
path.write_text(s)
print("configured v4.4.137 schematic pinmap: team-network sample selected, official SLE UART samples disabled, AT UART on UART3, ws2812 IO0 blink states, buzzer IO14 muted, gps UART1(IO17/18) mapped, adc IO5/IO12 channel5 battery sampling mapped, TP4054 CHRG IO2 active-low mapped, display IO7/9/8(cs-low)/13/10, central+peripheral enabled, LVGL event panel requested, HTTP WebUI auto-start enabled")
PY

"${ssh_cmd[@]}" "cd '$UBUNTU_SDK' && python3 build.py -c ws63-liteos-app -j'$BUILD_JOBS'"

"${ssh_cmd[@]}" "python3 - '$UBUNTU_SDK'" <<'PY'
from pathlib import Path
import sys

sdk = Path(sys.argv[1])
cfg_path = sdk / "build/config/target_config/ws63/menuconfig/acore/ws63_liteos_app.config"
map_path = sdk / "output/ws63/acore/ws63-liteos-app/ws63-liteos-app.map"
elf_path = sdk / "output/ws63/acore/ws63-liteos-app/ws63-liteos-app.elf"
app_source_path = sdk / "application/samples/products/sle_team_network/src/ws63_team_network_app.c"
ws2812_source_path = sdk / "application/samples/products/sle_team_network/src/ws63_ws2812.c"
proto_source_path = sdk / "third_party/sle_mesh/src/sle_team_node.c"

cfg = cfg_path.read_text(errors="replace")
map_text = map_path.read_text(errors="replace")
elf = elf_path.read_bytes()
app_source = app_source_path.read_text(errors="replace")
ws2812_source = ws2812_source_path.read_text(errors="replace")
proto_source = proto_source_path.read_text(errors="replace")

required_cfg = [
    "CONFIG_SAMPLE_SUPPORT_SLE_TEAM_NETWORK=y",
    "CONFIG_SLE_TEAM_WS2812_ENABLE=y",
    "CONFIG_SLE_TEAM_WS2812_PIN=0",
    "CONFIG_SLE_TEAM_BUZZER_ENABLE=y",
    "CONFIG_SLE_TEAM_BUZZER_PIN=14",
    "CONFIG_SLE_TEAM_BUZZER_ACTIVE_HIGH=y",
    "CONFIG_SLE_TEAM_BUZZER_MUTED=y",
    "# CONFIG_SLE_TEAM_GPS_ENABLE is not set",
    "CONFIG_SLE_TEAM_ADC_ENABLE=y",
    "CONFIG_SLE_TEAM_ADC_CTRL_PIN=5",
    "CONFIG_SLE_TEAM_ADC_VBAT_PIN=12",
    "CONFIG_SLE_TEAM_ADC_VBAT_CHANNEL=5",
    "CONFIG_SLE_TEAM_ADC_SAMPLE_SETTLE_MS=50",
    "CONFIG_SLE_TEAM_ADC_SAMPLE_INTERVAL_S=30",
    "CONFIG_SLE_TEAM_CHRG_ENABLE=y",
    "CONFIG_SLE_TEAM_CHRG_PIN=2",
    "CONFIG_SLE_TEAM_CHRG_ACTIVE_LOW=y",
    "CONFIG_SLE_TEAM_CHRG_EXTERNAL_PULLUP=y",
    "CONFIG_ADC_SUPPORT_AUTO_SCAN=y",
    "CONFIG_ADC_USING_V154=y",
    "CONFIG_SLE_TEAM_ST7789_ENABLE=y",
    "CONFIG_SLE_TEAM_ST7789_SCLK_PIN=7",
    "CONFIG_SLE_TEAM_ST7789_MOSI_PIN=9",
    "CONFIG_SLE_TEAM_ST7789_CS_PIN=8",
    "CONFIG_SLE_TEAM_ST7789_CS_ALWAYS_LOW=y",
    "CONFIG_SLE_TEAM_ST7789_DC_PIN=13",
    "CONFIG_SLE_TEAM_ST7789_RESET_PIN=10",
    "CONFIG_SLE_TEAM_DISPLAY_USE_LVGL=y",
]
for item in required_cfg:
    if item not in cfg:
        raise SystemExit(f"post-build guard failed: missing {item}")

for forbidden in [
    "CONFIG_SAMPLE_SUPPORT_SLE_UART=y",
    "CONFIG_SAMPLE_SUPPORT_SLE_UART_1_VS_8=y",
    "CONFIG_SAMPLE_SUPPORT_SLE_UART_CLIENT_1_VS_8=y",
    "CONFIG_SAMPLE_SUPPORT_SLE_UART_SERVER_1_VS_8=y",
]:
    if forbidden in cfg:
        raise SystemExit(f"post-build guard failed: official UART sample still enabled: {forbidden}")

required_map = [
    "ws63_team_network_app.c.obj",
    "ws63_st7789_display.c.obj",
    "ws63_ws2812.c.obj",
    "sle_team_node.c.obj",
]
for item in required_map:
    if item not in map_text:
        raise SystemExit(f"post-build guard failed: linked map missing {item}")

required_bytes = [
    b"v4.4.137",
    b"seek stop timeout, fallback connect pending",
    b"connect request addr:",
    b"cfg direct",
    b"runtimeDirectCap",
    b"runtimeRelayBudget",
    b"member upstream recover",
    b"plan=%u",
    b"[display] st7789 ready",
    b"[display] st7789 pins primed cs=%u held-low settle_ms=%u dc=%u rst=%u",
    b"phase=ready",
    b"timing=cycle-counter",
    b"boot rgb-test follows",
    b"panel gnd still needs real board ground",
    b"[display-event] event=%s label=%s member=%u",
    b"[team] boot unconfigured",
    b"[hw] init summary fw=%s",
    b"[hw] gps configured=%u present=0 ready=%u",
    b"[hw] adc present=%u ready=%u",
    b"[hw] chrg present=%u ready=%u",
    b"[battery] sample valid=%u",
    b"bat commands: status|sample",
    b"vbat_mv=%u",
    b"powerSource",
    b"battery-or-full",
    b"pwr-charging",
    b"api=power",
    b"chrgExternalPullup",
    b"[state] rgb state=%s",
    b"state=%s flash=%s",
    b"buzz muted by firmware",
    b"[cfg-json]",
    b"[team] disconnect lookup",
    b"already_offline=%u",
    b"relay failover begin",
    b"relay failover duplicate lost=%u",
    b"relay failover holding relay target",
    b"relay config notify pending",
    b"liveness preserved",
    b"direct cap prune confirmed",
    b"direct cap prune disconnect",
    b"force rescan stop seek ret",
    b"seek disabled for force rescan",
    b"pairing restart scan reason=%s",
    b"relay demote drop child conn=%u",
    b"member-leave",
    b"TeamDisplayTask",
    b"relay rebalance demand",
    b"relay swap observe",
    b"swap-promote",
    b"swap-demote",
    b"v3.2 schematic pinmap, muted buzzer",
]
for item in required_bytes:
    if item not in elf:
        raise SystemExit(f"post-build guard failed: ELF missing {item.decode('ascii', errors='replace')}")

for source_name, source_text, item in [
    ("sle_team_node.c", proto_source, "Route updates are leader policy hints"),
    ("ws63_team_network_app.c", app_source, "team_member_relay_can_accept_child"),
    ("ws63_team_network_app.c", app_source, "team_leader_enforce_direct_capacity"),
    ("ws63_team_network_app.c", app_source, "team_leader_pairing_restart_scan"),
    ("ws63_team_network_app.c", app_source, "team_member_drop_relay_children"),
    ("ws63_team_network_app.c", app_source, 'team_member_drop_relay_children("member-leave");'),
    ("ws63_team_network_app.c", app_source, "#define SLE_TEAM_MEMBER_UPSTREAM_RECOVER_INTERVAL_S 2U"),
    ("ws63_team_network_app.c", app_source, "#define SLE_TEAM_MEMBER_UPSTREAM_STUCK_S 8U"),
    ("ws63_team_network_app.c", app_source, "team_member_upstream_recover_after_tx_fail"),
    ("ws63_team_network_app.c", app_source, 'team_member_upstream_recover_after_tx_fail("not-ready"'),
    ("ws63_team_network_app.c", app_source, 'team_member_upstream_recover_after_tx_fail("write-fail"'),
    ("ws63_team_network_app.c", app_source, "team_member_upstream_recover_tick();"),
    ("ws63_team_network_app.c", app_source, "sle_uart_server_adv_restart();"),
    ("ws63_team_network_app.c", app_source, "sle_uart_server_disconnect_current();"),
    ("ws63_team_network_app.c", app_source, "stale routes through the lost relay"),
    ("ws63_team_network_app.c", app_source, "#define SLE_TEAM_RELAY_FAILOVER_GRACE_S 6U"),
    ("ws63_team_network_app.c", app_source, "#define SLE_TEAM_WS2812_IDLE_BLINK_ON_MS 500U"),
    ("ws63_team_network_app.c", app_source, "#define SLE_TEAM_WS2812_LEADER_BLINK_ON_MS 500U"),
    ("ws63_team_network_app.c", app_source, "#define SLE_TEAM_WS2812_MEMBER_BLINK_ON_MS 500U"),
    ("ws63_team_network_app.c", app_source, "#define SLE_TEAM_WS2812_ERROR_BLINK_ON_MS 220U"),
    ("ws63_team_network_app.c", app_source, "#define SLE_TEAM_WS2812_LEADER_BLINK_PERIOD_MS 1000U"),
    ("ws63_team_network_app.c", app_source, "#define SLE_TEAM_WS2812_ERROR_BLINK_PERIOD_MS 360U"),
    ("ws63_team_network_app.c", app_source, "#define SLE_TEAM_WS2812_FLASH_ON_MS 140U"),
    ("ws63_team_network_app.c", app_source, "team_rgb_state_is_blinking"),
    ("ws63_team_network_app.c", app_source, "ws2812_base_enter_ms = now_ms"),
    ("ws63_team_network_app.c", app_source, "team_rgb_state_blink_is_on(state, now_ms - g_team_rt.ws2812_base_enter_ms)"),
    ("ws63_team_network_app.c", app_source, "static void team_ws2812_restart_base_phase"),
    ("ws63_team_network_app.c", app_source, "team_ws2812_restart_base_phase(now_ms);"),
    ("ws63_team_network_app.c", app_source, "#define SLE_TEAM_WS2812_TEST_R 64U"),
    ("ws63_team_network_app.c", app_source, "team_ws2812_test_pattern();"),
    ("ws63_team_network_app.c", app_source, "timing=cycle-counter"),
    ("ws63_team_network_app.c", app_source, "team_ws2812_start_flash(TEAM_RGB_STATE_ERROR)"),
    ("ws63_team_network_app.c", app_source, "team_ws2812_start_flash(TEAM_RGB_STATE_LEADER)"),
    ("ws63_st7789_display.c", app_source_path.with_name("ws63_st7789_display.c").read_text(errors="replace"),
     "ST7789_CS_LOW_SETTLE_MS"),
    ("ws63_st7789_display.c", app_source_path.with_name("ws63_st7789_display.c").read_text(errors="replace"),
     "st7789 pins primed"),
    ("ws63_ws2812.c", ws2812_source, "rdcycle %0"),
    ("ws63_ws2812.c", ws2812_source, "WS63_WS2812_SLOT_CYCLES"),
]:
    if item not in source_text:
        raise SystemExit(f"post-build guard failed: source {source_name} missing {item}")

if "BREATHE" in app_source or "team_rgb_state_is_breathing" in app_source:
    raise SystemExit("post-build guard failed: WS2812 breathing path still present")

flash_source_start = app_source.index("static uint8_t team_ws2812_refresh_flash")
flash_source_end = app_source.index("static void team_ws2812_refresh_network_state", flash_source_start)
flash_source = app_source[flash_source_start:flash_source_end]
if flash_source.index("team_ws2812_restart_base_phase(now_ms);") > flash_source.index("team_ws2812_render_base_state(now_ms);"):
    raise SystemExit("post-build guard failed: WS2812 flash completion restores base state before restarting blink phase")

print("post-build guard passed: team-network app, RGB blink states, ST7789/LVGL display, version strings, and direct-cap source guards are present")
PY

mkdir -p "$(dirname "$LOCAL_OUT")"
rsync -az -e "${rsync_ssh[*]}" "$UBUNTU_USER@$UBUNTU_HOST:$REMOTE_PKG" "$ARCHIVE_OUT"
cp "$ARCHIVE_OUT" "$LOCAL_OUT"
ls -lh "$ARCHIVE_OUT" "$LOCAL_OUT"
