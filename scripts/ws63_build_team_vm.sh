#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

export UBUNTU_HOST="${VM_HOST:-${UBUNTU_HOST:-127.0.0.1}}"
export UBUNTU_PORT="${VM_PORT:-${UBUNTU_PORT:-2222}}"
export UBUNTU_USER="${VM_USER:-${UBUNTU_USER:-codex}}"
export UBUNTU_PASS="${VM_PASS:-${UBUNTU_PASS:-codex}}"
export UBUNTU_SDK="${VM_SDK:-${UBUNTU_SDK:-/home/codex/workspace/bearpi-pico_h3863_fresh}}"

exec "$ROOT_DIR/scripts/ws63_build_team_ubuntu.sh" "$@"
