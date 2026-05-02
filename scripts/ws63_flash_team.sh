#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BURN_TOOL="${BURN_TOOL:-/Users/bh4me_macair/Library/Python/3.9/bin/burn}"
FW_ROOT="${FW_ROOT:-/Users/bh4me_macair/Documents/Codex/bearpi-pico_h3863/output_from_vm}"

usage() {
  cat <<'USAGE'
Usage:
  scripts/ws63_flash_team.sh leader [port]
  scripts/ws63_flash_team.sh member [port]

Defaults:
  leader port: /dev/tty.usbserial-10
  member port: /dev/tty.usbserial-110

Environment:
  BURN_TOOL=/path/to/burn
  FW_ROOT=/path/to/output_from_vm

The script prints role, port, and firmware path, then asks for an exact
confirmation before it runs the burn tool.
USAGE
}

role="${1:-}"
case "$role" in
  leader)
    default_port="/dev/tty.usbserial-10"
    firmware="$FW_ROOT/team_network_leader_unified_sle/ws63-liteos-app_leader_all.fwpkg"
    fallback_firmware="$FW_ROOT/team_network_leader_serial_led/ws63-liteos-app_leader_all.fwpkg"
    ;;
  member)
    default_port="/dev/tty.usbserial-110"
    firmware="$FW_ROOT/team_network_member_unified_sle/ws63-liteos-app_member_all.fwpkg"
    fallback_firmware="$FW_ROOT/team_network_member_serial_led/ws63-liteos-app_member_all.fwpkg"
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

port="${2:-$default_port}"

echo "WS63 team flash confirmation"
echo "repo:     $ROOT_DIR"
echo "role:     $role"
echo "port:     $port"
echo "firmware: $firmware"
echo

if [[ ! -x "$BURN_TOOL" ]]; then
  echo "Burn tool is not executable: $BURN_TOOL" >&2
  exit 1
fi

if [[ ! -e "$port" ]]; then
  echo "Serial port does not exist: $port" >&2
  echo "Current likely serial ports:" >&2
  ls -1 /dev/tty.* /dev/cu.* 2>/dev/null | sed -n '/usb\|wch\|serial\|SLAB\|UART\|modem/Ip' >&2 || true
  exit 1
fi

if [[ ! -f "$firmware" ]]; then
  if [[ -f "$fallback_firmware" ]]; then
    echo "Unified firmware package not found, using previous serial LED package:"
    echo "fallback: $fallback_firmware"
    firmware="$fallback_firmware"
  else
    echo "Firmware package does not exist: $firmware" >&2
    echo "Fallback package does not exist: $fallback_firmware" >&2
    exit 1
  fi
fi

expected="flash $role"
printf "Type '%s' to continue: " "$expected"
read -r answer
if [[ "$answer" != "$expected" ]]; then
  echo "Cancelled."
  exit 1
fi

echo "Starting burn. When the tool waits for reset, press BOOT + RESET as needed."
exec "$BURN_TOOL" -p "$port" -b 115200 "$firmware"
