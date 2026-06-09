#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'USAGE'
Usage:
  scripts/build/ws63_build_v4_local_wsl.sh unified

Builds the v4 WS63 team firmware inside local WSL using the SDK on the
Windows filesystem.

Environment:
  WSL_SDK=/mnt/d/bearpi-pico_h3863-master
  OUT_ROOT=/path/to/sle/output_from_vm
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
WSL_SDK="${WSL_SDK:-/mnt/d/bearpi-pico_h3863-master}"
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

CONFIG_PATH="$WSL_SDK/build/config/target_config/ws63/menuconfig/acore/ws63_liteos_app.config"
REMOTE_PKG="$WSL_SDK/output/ws63/fwpkg/ws63-liteos-app/ws63-liteos-app_all.fwpkg"
REMOTE_PROTO="$WSL_SDK/third_party/sle_mesh"
REMOTE_APP="$WSL_SDK/application/samples/products/sle_team_network"
LOCAL_OUT="$OUT_ROOT/$out_dir/$out_name"
ARCHIVE_OUT="$(next_archive_path "$LOCAL_OUT" "v4.4.125")"

export PATH="$HOME/.local/bin:$PATH"

echo "WS63 local WSL build"
echo "profile:    v4.4.125 unified runtime role (v3.2 schematic pinmap + ADC battery)"
echo "sdk:        $WSL_SDK"
echo "archive:    $ARCHIVE_OUT"
echo "latest:     $LOCAL_OUT"
echo

command -v cmake >/dev/null || { echo "cmake not found in PATH=$PATH" >&2; exit 2; }
command -v ninja >/dev/null || { echo "ninja not found in PATH=$PATH" >&2; exit 2; }
test -f "$CONFIG_PATH" || { echo "config not found: $CONFIG_PATH" >&2; exit 2; }

mkdir -p "$REMOTE_PROTO" "$REMOTE_APP"
rsync -a --delete --exclude '.git' --exclude 'build' --exclude 'dist' --exclude 'node_modules' \
  "$ROOT_DIR/include/" "$REMOTE_PROTO/include/"
rsync -a --delete --exclude '.git' --exclude 'build' --exclude 'dist' --exclude 'node_modules' \
  "$ROOT_DIR/src/" "$REMOTE_PROTO/src/"
rsync -a --delete --exclude '.git' --exclude 'build' --exclude 'dist' --exclude 'node_modules' \
  "$ROOT_DIR/xc/ws63_team_network/" "$REMOTE_APP/"

python3 - "$CONFIG_PATH" "$self_id" <<'PY'
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
for key in [
    "CONFIG_SAMPLE_SUPPORT_SLE_UART",
    "CONFIG_SAMPLE_SUPPORT_SLE_UART_SERVER",
    "CONFIG_SAMPLE_SUPPORT_SLE_UART_CLIENT",
    "CONFIG_SAMPLE_SUPPORT_SLE_UART_1_VS_8",
    "CONFIG_SAMPLE_SUPPORT_SLE_UART_SERVER_1_VS_8",
    "CONFIG_SAMPLE_SUPPORT_SLE_UART_CLIENT_1_VS_8",
    "CONFIG_SAMPLE_SUPPORT_BLE_UART",
    "CONFIG_SAMPLE_SUPPORT_SLE_GETAWAY",
]:
    s = unset_kconfig_bool(s, key)
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
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_HEARTBEAT_TIMEOUT_S", "4")
s = set_kconfig_value(s, "CONFIG_SPI_SUPPORT_MASTER", "y")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_WIFI_AP_ENABLE", "y")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_WIFI_AP_AUTO_START", "y")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_WIFI_AP_SSID", '"SLE-TEAM-V4"')
s = set_kconfig_value(s, "CONFIG_SUPPORT_SLE_PERIPHERAL", "y")
s = set_kconfig_value(s, "CONFIG_SUPPORT_SLE_CENTRAL", "y")
path.write_text(s)
print("configured v4.4.125 local WSL pinmap, ADC battery sampling, and team-network sample")
PY

cd "$WSL_SDK"
python3 build.py -c ws63-liteos-app -j"$BUILD_JOBS"

python3 - "$WSL_SDK" <<'PY'
from pathlib import Path
import sys

sdk = Path(sys.argv[1])
cfg_path = sdk / "build/config/target_config/ws63/menuconfig/acore/ws63_liteos_app.config"
map_path = sdk / "output/ws63/acore/ws63-liteos-app/ws63-liteos-app.map"
elf_path = sdk / "output/ws63/acore/ws63-liteos-app/ws63-liteos-app.elf"
cfg = cfg_path.read_text(errors="replace")
map_text = map_path.read_text(errors="replace")
elf = elf_path.read_bytes()

for item in [
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
]:
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
for item in [
    "ws63_team_network_app.c.obj",
    "ws63_st7789_display.c.obj",
    "sle_team_node.c.obj",
]:
    if item not in map_text:
        raise SystemExit(f"post-build guard failed: linked map missing {item}")
for item in [
    b"v4.4.125",
    b"seek stop timeout, fallback connect pending",
    b"connect request addr:",
    b"cfg direct",
    b"runtimeDirectCap",
    b"runtimeRelayBudget",
    b"plan=%u",
    b"[display] st7789 ready",
    b"phase=ready",
    b"[display-event] event=%s label=%s member=%u",
    b"[team] boot unconfigured",
    b"[hw] init summary fw=%s",
    b"[hw] gps configured=%u present=0 ready=%u",
    b"[hw] adc present=%u ready=%u",
    b"[battery] sample valid=%u",
    b"bat commands: status|sample",
    b"vbat_mv=%u",
    b"[state] rgb state=%s",
    b"state=%s flash=%s",
    b"buzz muted by firmware",
    b"[cfg-json]",
    b"[team] disconnect lookup",
    b"already_offline=%u",
    b"TeamDisplayTask",
    b"relay rebalance demand",
    b"relay swap observe",
    b"swap-promote",
    b"swap-demote",
    b"v3.2 schematic pinmap, muted buzzer",
]:
    if item not in elf:
        raise SystemExit(f"post-build guard failed: ELF missing {item.decode('ascii', errors='replace')}")
print("post-build guard passed: local WSL v4.4.125")
PY

mkdir -p "$(dirname "$LOCAL_OUT")"
cp "$REMOTE_PKG" "$ARCHIVE_OUT"
cp "$ARCHIVE_OUT" "$LOCAL_OUT"
ls -lh "$ARCHIVE_OUT" "$LOCAL_OUT"
