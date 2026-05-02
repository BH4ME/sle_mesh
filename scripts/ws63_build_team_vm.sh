#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'USAGE'
Usage:
  scripts/ws63_build_team_vm.sh leader
  scripts/ws63_build_team_vm.sh member

Builds the WS63 team firmware inside the local Ubuntu VM and copies the
resulting .fwpkg back to the Mac output_from_vm directory.

Environment:
  VM_HOST=127.0.0.1
  VM_PORT=2222
  VM_USER=codex
  VM_PASS=codex
  VM_SDK=/home/codex/workspace/bearpi-pico_h3863_fresh
  OUT_ROOT=/Users/bh4me_macair/Documents/Codex/bearpi-pico_h3863/output_from_vm
USAGE
}

role="${1:-}"
case "$role" in
  leader)
    self_id=1
    leader_config="CONFIG_SLE_TEAM_NODE_IS_LEADER=y"
    leader_unset="# CONFIG_SLE_TEAM_NODE_IS_LEADER is not set"
    out_dir="team_network_leader_unified_sle"
    out_name="ws63-liteos-app_leader_all.fwpkg"
    ;;
  member)
    self_id=2
    leader_config="# CONFIG_SLE_TEAM_NODE_IS_LEADER is not set"
    leader_unset="CONFIG_SLE_TEAM_NODE_IS_LEADER=y"
    out_dir="team_network_member_unified_sle"
    out_name="ws63-liteos-app_member_all.fwpkg"
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

VM_HOST="${VM_HOST:-127.0.0.1}"
VM_PORT="${VM_PORT:-2222}"
VM_USER="${VM_USER:-codex}"
VM_PASS="${VM_PASS:-codex}"
VM_SDK="${VM_SDK:-/home/codex/workspace/bearpi-pico_h3863_fresh}"
OUT_ROOT="${OUT_ROOT:-/Users/bh4me_macair/Documents/Codex/bearpi-pico_h3863/output_from_vm}"
CONFIG_PATH="$VM_SDK/build/config/target_config/ws63/menuconfig/acore/ws63_liteos_app.config"
REMOTE_PKG="$VM_SDK/output/ws63/fwpkg/ws63-liteos-app/ws63-liteos-app_all.fwpkg"
LOCAL_OUT="$OUT_ROOT/$out_dir/$out_name"

ssh_cmd=(sshpass -p "$VM_PASS" ssh -p "$VM_PORT" "$VM_USER@$VM_HOST")
scp_cmd=(sshpass -p "$VM_PASS" scp -P "$VM_PORT")

echo "WS63 VM build"
echo "role:       $role"
echo "self id:    $self_id"
echo "vm sdk:     $VM_SDK"
echo "local out:  $LOCAL_OUT"
echo

"${ssh_cmd[@]}" "test -f '$CONFIG_PATH'"

"${ssh_cmd[@]}" "python3 - '$CONFIG_PATH' '$role' '$self_id' '$leader_config' '$leader_unset'" <<'PY'
from pathlib import Path
import sys

path = Path(sys.argv[1])
role = sys.argv[2]
self_id = sys.argv[3]
leader_config = sys.argv[4]
leader_unset = sys.argv[5]
s = path.read_text()

if leader_unset in s:
    s = s.replace(leader_unset, leader_config)
elif leader_config not in s:
    s += "\n" + leader_config + "\n"

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

s = set_kconfig_value(s, "CONFIG_SLE_TEAM_SELF_ID", self_id)
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_LEADER_ID", "1")
s = set_kconfig_value(s, "CONFIG_SUPPORT_SLE_PERIPHERAL", "y")
s = set_kconfig_value(s, "CONFIG_SUPPORT_SLE_CENTRAL", "y")
path.write_text(s)
print(f"configured {role} self={self_id} with central+peripheral enabled")
PY

"${ssh_cmd[@]}" "cd '$VM_SDK' && python3 build.py ws63-liteos-app -j4"

mkdir -p "$(dirname "$LOCAL_OUT")"
"${scp_cmd[@]}" "$VM_USER@$VM_HOST:$REMOTE_PKG" "$LOCAL_OUT"
ls -lh "$LOCAL_OUT"
