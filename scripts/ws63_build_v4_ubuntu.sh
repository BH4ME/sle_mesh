#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'USAGE'
Usage:
  scripts/ws63_build_v4_ubuntu.sh unified

Builds the v4 WS63 team firmware on a LAN Ubuntu build machine.

Environment is the same as scripts/ws63_build_team_ubuntu.sh:
  UBUNTU_HOST=192.168.1.50
  UBUNTU_PORT=22
  UBUNTU_USER=codex
  UBUNTU_PASS=codex
  UBUNTU_SDK=/home/codex/workspace/bearpi-pico_h3863_fresh
  OUT_ROOT=/Users/bh4me_macair/Documents/Codex/bearpi-pico_h3863/output_from_vm
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

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
UBUNTU_HOST="${UBUNTU_HOST:-}"
UBUNTU_PORT="${UBUNTU_PORT:-22}"
UBUNTU_USER="${UBUNTU_USER:-codex}"
UBUNTU_PASS="${UBUNTU_PASS:-}"
UBUNTU_SDK="${UBUNTU_SDK:-/home/codex/workspace/bearpi-pico_h3863_fresh}"
OUT_ROOT="${OUT_ROOT:-/Users/bh4me_macair/Documents/Codex/bearpi-pico_h3863/output_from_vm}"
BUILD_JOBS="${BUILD_JOBS:-4}"

if [[ -z "$UBUNTU_HOST" ]]; then
  echo "UBUNTU_HOST is required, for example: UBUNTU_HOST=192.168.1.50 $0 $role" >&2
  exit 2
fi

CONFIG_PATH="$UBUNTU_SDK/build/config/target_config/ws63/menuconfig/acore/ws63_liteos_app.config"
REMOTE_PKG="$UBUNTU_SDK/output/ws63/fwpkg/ws63-liteos-app/ws63-liteos-app_all.fwpkg"
REMOTE_PROTO="$UBUNTU_SDK/third_party/sle_mesh"
REMOTE_APP="$UBUNTU_SDK/application/samples/products/sle_team_network"
LOCAL_OUT="$OUT_ROOT/$out_dir/$out_name"
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
echo "profile:    v4.4.37 unified runtime role (persistent cfg flush guard + LVGL patch idempotence)"
echo "fallback id:$self_id"
echo "host:       $UBUNTU_USER@$UBUNTU_HOST:$UBUNTU_PORT"
echo "sdk:        $UBUNTU_SDK"
echo "local out:  $LOCAL_OUT"
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
s = unset_kconfig_bool(s, "CONFIG_SLE_TEAM_BUZZER_ENABLE")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_BUZZER_PIN", "14")
s = unset_kconfig_bool(s, "CONFIG_SLE_TEAM_BUZZER_ACTIVE_HIGH")
s = unset_kconfig_bool(s, "CONFIG_SLE_TEAM_GPS_ENABLE")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_GPS_UART_BUS", "1")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_GPS_UART_TXD_PIN", "17")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_GPS_UART_RXD_PIN", "18")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_ST7789_ENABLE", "y")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_ST7789_SPI_BUS", "0")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_ST7789_SCLK_PIN", "6")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_ST7789_MOSI_PIN", "8")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_ST7789_CS_PIN", "7")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_ST7789_DC_PIN", "9")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_ST7789_RESET_PIN", "13")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_ST7789_X_OFFSET", "40")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_ST7789_Y_OFFSET", "53")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_ST7789_WIDTH", "240")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_ST7789_HEIGHT", "135")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_DISPLAY_USE_LVGL", "y")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_LVGL_DRAW_BUF_LINES", "8")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_HEARTBEAT_INTERVAL_S", "1")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_HEARTBEAT_TIMEOUT_S", "4")
s = set_kconfig_value(s, "CONFIG_SPI_SUPPORT_MASTER", "y")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_WIFI_AP_ENABLE", "y")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_WIFI_AP_AUTO_START", "y")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_WIFI_AP_SSID", '"SLE-TEAM-V4"')
s = set_kconfig_value(s, "CONFIG_SUPPORT_SLE_PERIPHERAL", "y")
s = set_kconfig_value(s, "CONFIG_SUPPORT_SLE_CENTRAL", "y")
path.write_text(s)
print("configured v4.4.37 schematic pinmap: team-network sample selected, official SLE UART samples disabled, AT UART on UART3, ws2812 IO0, buzzer default disabled (IO14 safe off), gps UART1(IO17/18) mapped, central+peripheral enabled, LVGL requested, HTTP WebUI auto-start enabled")
PY

"${ssh_cmd[@]}" "cd '$UBUNTU_SDK' && python3 build.py -c ws63-liteos-app -j'$BUILD_JOBS'"

"${ssh_cmd[@]}" "python3 - '$UBUNTU_SDK'" <<'PY'
from pathlib import Path
import sys

sdk = Path(sys.argv[1])
cfg_path = sdk / "build/config/target_config/ws63/menuconfig/acore/ws63_liteos_app.config"
map_path = sdk / "output/ws63/acore/ws63-liteos-app/ws63-liteos-app.map"
elf_path = sdk / "output/ws63/acore/ws63-liteos-app/ws63-liteos-app.elf"

cfg = cfg_path.read_text(errors="replace")
map_text = map_path.read_text(errors="replace")
elf = elf_path.read_bytes()

required_cfg = [
    "CONFIG_SAMPLE_SUPPORT_SLE_TEAM_NETWORK=y",
    "CONFIG_SLE_TEAM_ST7789_ENABLE=y",
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
    "sle_team_node.c.obj",
]
for item in required_map:
    if item not in map_text:
        raise SystemExit(f"post-build guard failed: linked map missing {item}")

required_bytes = [
    b"v4.4.37",
    b"[display] st7789 ready",
    b"[team] boot unconfigured",
    b"[cfg-json]",
]
for item in required_bytes:
    if item not in elf:
        raise SystemExit(f"post-build guard failed: ELF missing {item.decode('ascii', errors='replace')}")

print("post-build guard passed: team-network app, ST7789 display, version, and serial cfg strings are linked")
PY

mkdir -p "$(dirname "$LOCAL_OUT")"
rsync -az -e "${rsync_ssh[*]}" "$UBUNTU_USER@$UBUNTU_HOST:$REMOTE_PKG" "$LOCAL_OUT"
ls -lh "$LOCAL_OUT"
