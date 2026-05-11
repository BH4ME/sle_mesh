#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
LOG_ROOT="$ROOT_DIR/logs/auto_test"
TS="$(date +%Y%m%d_%H%M%S)"
RUN_DIR="$LOG_ROOT/$TS"
REPORT="$RUN_DIR/report.md"
SUMMARY="$RUN_DIR/summary.txt"

DO_UNIT=1
DO_SIM=1
DO_BURN=0
DO_SERIAL=0
DO_LINK_CYCLE=0
DO_ROLE_BIND=0
SKIP_BUILD=0
SUITE="all"
SIM_STRESS=10
PY_STRESS=20
SERIAL_SECONDS=20
SERIAL_PORTS=""
FLASH_ROLE="unified"
LINK_BOOTSTRAP_ROLES=1
LINK_TEAM_ID=1
LINK_LEADER_ID=-1
LINK_MEMBER_ID=2
LINK_CHANNEL=17
LINK_LEADER_SUFFIX="${LINK_LEADER_SUFFIX:-}"
ROLE_BIND_MEMBER_JOIN=0
ROLE_BIND_WITH_FLASH=0

die() {
  echo "[auto-test] ERROR: $*" >&2
  exit 1
}

usage() {
  cat <<USAGE
Usage:
  automation/ws63/scripts/ws63_test_system.sh [options]

Options:
  --quick                    run unit + sim(all stress=5), no burn
  --full                     run unit + sim(all stress=10) + py stress
  --with-burn                include firmware burn stage (non-interactive)
  --with-serial              include serial smoke capture stage
  --with-link-cycle          include serial connect->leave->reconnect test
  --with-role-bind           include post-flash role bind stage (one leader -> all members)
  --role-bind-with-flash     run flash inside role-bind stage (use instead of --with-burn to avoid duplicate flashing)
  --ports <p1,p2,...>        serial ports for burn/serial stage
  --flash-role <role>        leader|member|unified (default unified)
  --sim-stress <n>           stress for simulate_v2.sh all suite
  --py-stress <n>            stress for python suite
  --serial-seconds <n>       per-port capture seconds (default 20)
  --link-team-id <id>        team id used by link-cycle (default 1)
  --link-leader-id <id>      leader id used by link-cycle (default -1: auto)
  --link-member-id <id>      member id used by link-cycle (default 2)
  --link-channel <id>        channel used by link-cycle (default 17)
  --link-no-bootstrap-roles  skip role bootstrap before link-cycle
  --link-leader-suffix <hex> leader MAC suffix (4 hex chars) for bootstrap, auto-detect if omitted
  --role-bind-member-join    send join after role member in role-bind stage
  --skip-build               skip remote build command in burn stage
  -h, --help                 show this help

Environment used by burn/build stage:
  UBUNTU_HOST, UBUNTU_USER, UBUNTU_PASS, UBUNTU_SDK, BUILD_JOBS
USAGE
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --quick)
      DO_UNIT=1; DO_SIM=1; DO_BURN=0; DO_SERIAL=0; SIM_STRESS=5; PY_STRESS=10; shift ;;
    --full)
      DO_UNIT=1; DO_SIM=1; DO_BURN=0; DO_SERIAL=0; SIM_STRESS=10; PY_STRESS=20; shift ;;
    --with-burn)
      DO_BURN=1; shift ;;
    --with-serial)
      DO_SERIAL=1; shift ;;
    --with-link-cycle)
      DO_LINK_CYCLE=1; shift ;;
    --with-role-bind)
      DO_ROLE_BIND=1; shift ;;
    --role-bind-with-flash)
      ROLE_BIND_WITH_FLASH=1; shift ;;
    --ports)
      SERIAL_PORTS="${2:-}"; shift 2 ;;
    --flash-role)
      FLASH_ROLE="${2:-}"; shift 2 ;;
    --sim-stress)
      SIM_STRESS="${2:-}"; shift 2 ;;
    --py-stress)
      PY_STRESS="${2:-}"; shift 2 ;;
    --serial-seconds)
      SERIAL_SECONDS="${2:-}"; shift 2 ;;
    --link-team-id)
      LINK_TEAM_ID="${2:-}"; shift 2 ;;
    --link-leader-id)
      LINK_LEADER_ID="${2:-}"; shift 2 ;;
    --link-member-id)
      LINK_MEMBER_ID="${2:-}"; shift 2 ;;
    --link-channel)
      LINK_CHANNEL="${2:-}"; shift 2 ;;
    --link-no-bootstrap-roles)
      LINK_BOOTSTRAP_ROLES=0; shift ;;
    --link-leader-suffix)
      LINK_LEADER_SUFFIX="${2:-}"; shift 2 ;;
    --role-bind-member-join)
      ROLE_BIND_MEMBER_JOIN=1; shift ;;
    --skip-build)
      SKIP_BUILD=1; shift ;;
    -h|--help)
      usage; exit 0 ;;
    *)
      die "unknown option: $1" ;;
  esac
done

[[ "$SIM_STRESS" =~ ^[0-9]+$ ]] || die "--sim-stress must be integer"
[[ "$PY_STRESS" =~ ^[0-9]+$ ]] || die "--py-stress must be integer"
[[ "$SERIAL_SECONDS" =~ ^[0-9]+$ ]] || die "--serial-seconds must be integer"
[[ "$LINK_TEAM_ID" =~ ^[0-9]+$ ]] || die "--link-team-id must be integer"
[[ "$LINK_LEADER_ID" =~ ^-?[0-9]+$ ]] || die "--link-leader-id must be integer"
[[ "$LINK_MEMBER_ID" =~ ^[0-9]+$ ]] || die "--link-member-id must be integer"
[[ "$LINK_CHANNEL" =~ ^[0-9]+$ ]] || die "--link-channel must be integer"
if [[ -n "$LINK_LEADER_SUFFIX" ]] && [[ ! "$LINK_LEADER_SUFFIX" =~ ^[0-9A-Fa-f]{4}$ ]]; then
  die "--link-leader-suffix must be 4 hex digits"
fi

mkdir -p "$RUN_DIR"

log_step() {
  local name="$1"
  printf "\n## %s\n" "$name" | tee -a "$REPORT"
}

run_cmd() {
  local name="$1"
  shift
  local log="$RUN_DIR/${name}.log"
  echo "[auto-test] running: $name" | tee -a "$SUMMARY"
  if "$@" >"$log" 2>&1; then
    echo "PASS $name" | tee -a "$SUMMARY"
    printf -- "- %s: PASS\n" "$name" >> "$REPORT"
  else
    echo "FAIL $name (see $log)" | tee -a "$SUMMARY"
    printf -- "- %s: FAIL (log: %s)\n" "$name" "$log" >> "$REPORT"
    return 1
  fi
}

{
  echo "# WS63 Auto Test Report"
  echo
  echo "- Timestamp: $TS"
  echo "- Run Dir: $RUN_DIR"
  echo "- Config: unit=$DO_UNIT sim=$DO_SIM burn=$DO_BURN serial=$DO_SERIAL sim_stress=$SIM_STRESS py_stress=$PY_STRESS"
} > "$REPORT"

FAIL_COUNT=0

if [[ "$DO_UNIT" == "1" ]]; then
  log_step "Unit Tests"
  run_cmd "unit_python_sim" python3 -m unittest tools/test_sle_team_python_sim.py || FAIL_COUNT=$((FAIL_COUNT + 1))
  run_cmd "unit_ws63_auto_burn" python3 -m unittest automation/ws63/tests/test_ws63_auto_burn.py || FAIL_COUNT=$((FAIL_COUNT + 1))
fi

if [[ "$DO_SIM" == "1" ]]; then
  log_step "Simulation"
  run_cmd "sim_all" "$ROOT_DIR/scripts/simulate_v2.sh" --suite=all --stress="$SIM_STRESS" || FAIL_COUNT=$((FAIL_COUNT + 1))
  run_cmd "sim_python_stress" "$ROOT_DIR/scripts/simulate_v2.sh" --suite=python --stress="$PY_STRESS" --py-members=20 --py-direct-cap=8 --py-relay-target=3 --py-fail-tick=6 --py-recover-tick=10 --py-ticks=18 --py-packet-loss-rate=0.2 --py-jitter-min-ms=10 --py-jitter-max-ms=120 --py-batch-fail-relay-count=1 --py-batch-fail-relay-ticks=6 || FAIL_COUNT=$((FAIL_COUNT + 1))
fi

if [[ "$DO_BURN" == "1" ]]; then
  log_step "Build & Burn"
  if [[ "$SKIP_BUILD" != "1" ]]; then
    run_cmd "build_ubuntu" env UBUNTU_HOST="${UBUNTU_HOST:-}" UBUNTU_USER="${UBUNTU_USER:-}" UBUNTU_PASS="${UBUNTU_PASS:-}" UBUNTU_SDK="${UBUNTU_SDK:-}" BUILD_JOBS="${BUILD_JOBS:-4}" "$ROOT_DIR/scripts/ws63_build_team_ubuntu.sh" unified || FAIL_COUNT=$((FAIL_COUNT + 1))
  fi

  if [[ -z "$SERIAL_PORTS" ]]; then
    echo "[auto-test] burn stage skipped flashing because --ports not provided" | tee -a "$SUMMARY"
    printf -- "- burn_flash: SKIP (no --ports)\n" >> "$REPORT"
  else
    IFS=',' read -r -a PORT_ARR <<< "$SERIAL_PORTS"
    for port in "${PORT_ARR[@]}"; do
      port="${port// /}"
      [[ -n "$port" ]] || continue
      run_cmd "flash_${FLASH_ROLE}_$(basename "$port")" env WS63_FLASH_NO_CONFIRM=1 "$ROOT_DIR/scripts/ws63_flash_team.sh" --yes "$FLASH_ROLE" "$port" || FAIL_COUNT=$((FAIL_COUNT + 1))
    done
  fi
fi

if [[ "$DO_SERIAL" == "1" ]]; then
  log_step "Serial Smoke"
  [[ -n "$SERIAL_PORTS" ]] || { echo "[auto-test] serial stage requested but no --ports" | tee -a "$SUMMARY"; FAIL_COUNT=$((FAIL_COUNT + 1)); }
  if [[ -n "$SERIAL_PORTS" ]]; then
    IFS=',' read -r -a PORT_ARR <<< "$SERIAL_PORTS"
    for port in "${PORT_ARR[@]}"; do
      port="${port// /}"
      [[ -n "$port" ]] || continue
      cap_log="$RUN_DIR/serial_$(basename "$port").log"
      if command -v timeout >/dev/null 2>&1; then
        timeout "$SERIAL_SECONDS" cat "$port" > "$cap_log" 2>&1 || true
      else
        python3 - "$port" "$SERIAL_SECONDS" "$cap_log" <<'PY'
import serial, sys, time
port, seconds, out = sys.argv[1], int(sys.argv[2]), sys.argv[3]
end = time.time() + seconds
with serial.Serial(port, 115200, timeout=0.2) as ser, open(out, 'w', encoding='utf-8', errors='ignore') as f:
    while time.time() < end:
        data = ser.read(256)
        if data:
            f.write(data.decode('utf-8', errors='ignore'))
PY
      fi
      echo "[auto-test] serial captured: $cap_log" | tee -a "$SUMMARY"
      printf -- "- serial_%s: CAPTURED (%s sec)\n" "$(basename "$port")" "$SERIAL_SECONDS" >> "$REPORT"
    done
  fi
fi

if [[ "$DO_LINK_CYCLE" == "1" ]]; then
  log_step "Link Cycle"
  if [[ -z "$SERIAL_PORTS" ]]; then
    echo "[auto-test] link-cycle requested but no --ports" | tee -a "$SUMMARY"
    printf -- "- link_cycle: FAIL (no --ports)\\n" >> "$REPORT"
    FAIL_COUNT=$((FAIL_COUNT + 1))
  else
    IFS=',' read -r -a PORT_ARR <<< "$SERIAL_PORTS"
    if [[ "${#PORT_ARR[@]}" -lt 2 ]]; then
      echo "[auto-test] link-cycle requires two ports: leader,member" | tee -a "$SUMMARY"
      printf -- "- link_cycle: FAIL (need two ports)\\n" >> "$REPORT"
      FAIL_COUNT=$((FAIL_COUNT + 1))
    else
      LEADER_PORT="${PORT_ARR[0]// /}"
      MEMBER_PORT="${PORT_ARR[1]// /}"
      link_cmd=(
        python3 "$ROOT_DIR/automation/ws63/tools/ws63_link_cycle_test.py"
        --leader-port "$LEADER_PORT"
        --member-port "$MEMBER_PORT"
        --team-id "$LINK_TEAM_ID"
        --leader-id "$LINK_LEADER_ID"
        --member-id "$LINK_MEMBER_ID"
        --channel "$LINK_CHANNEL"
        --log-dir "$RUN_DIR"
      )
      if [[ "$LINK_BOOTSTRAP_ROLES" == "1" ]]; then
        link_cmd+=(--bootstrap-roles)
        if [[ -n "$LINK_LEADER_SUFFIX" ]]; then
          link_cmd+=(--leader-suffix "$LINK_LEADER_SUFFIX")
        fi
      fi
      run_cmd "link_cycle" "${link_cmd[@]}" || FAIL_COUNT=$((FAIL_COUNT + 1))
    fi
  fi
fi

if [[ "$DO_ROLE_BIND" == "1" ]]; then
  log_step "Role Bind"
  if [[ -z "$SERIAL_PORTS" ]]; then
    echo "[auto-test] role-bind requested but no --ports" | tee -a "$SUMMARY"
    printf -- "- role_bind: FAIL (no --ports)\\n" >> "$REPORT"
    FAIL_COUNT=$((FAIL_COUNT + 1))
  else
    IFS=',' read -r -a PORT_ARR <<< "$SERIAL_PORTS"
    if [[ "${#PORT_ARR[@]}" -lt 2 ]]; then
      echo "[auto-test] role-bind requires one leader + at least one member port" | tee -a "$SUMMARY"
      printf -- "- role_bind: FAIL (need >=2 ports)\\n" >> "$REPORT"
      FAIL_COUNT=$((FAIL_COUNT + 1))
    else
      LEADER_PORT="${PORT_ARR[0]// /}"
      MEMBER_PORTS_CSV=""
      for ((i=1; i<${#PORT_ARR[@]}; i++)); do
        p="${PORT_ARR[$i]// /}"
        [[ -n "$p" ]] || continue
        if [[ -z "$MEMBER_PORTS_CSV" ]]; then
          MEMBER_PORTS_CSV="$p"
        else
          MEMBER_PORTS_CSV="$MEMBER_PORTS_CSV,$p"
        fi
      done
      if [[ -z "$MEMBER_PORTS_CSV" ]]; then
        echo "[auto-test] role-bind has no member ports after parsing --ports" | tee -a "$SUMMARY"
        printf -- "- role_bind: FAIL (no member ports)\\n" >> "$REPORT"
        FAIL_COUNT=$((FAIL_COUNT + 1))
      else
        bind_cmd=(
          python3 "$ROOT_DIR/automation/ws63/tools/ws63_flash_bind_team.py"
          --leader-port "$LEADER_PORT"
          --member-ports "$MEMBER_PORTS_CSV"
          --baudrate 115200
          --team-id "$LINK_TEAM_ID"
          --channel "$LINK_CHANNEL"
        )
        if [[ "$ROLE_BIND_WITH_FLASH" == "1" ]]; then
          bind_cmd+=(--flash --flash-script "$ROOT_DIR/scripts/ws63_flash_team.sh" --flash-role "$FLASH_ROLE")
        fi
        if [[ -n "$LINK_LEADER_SUFFIX" ]]; then
          bind_cmd+=(--leader-suffix "$LINK_LEADER_SUFFIX")
        fi
        if [[ "$ROLE_BIND_MEMBER_JOIN" == "1" ]]; then
          bind_cmd+=(--member-join)
        fi
        run_cmd "role_bind" "${bind_cmd[@]}" || FAIL_COUNT=$((FAIL_COUNT + 1))
      fi
    fi
  fi
fi

log_step "Result"
if [[ "$FAIL_COUNT" -eq 0 ]]; then
  echo "PASS" | tee -a "$SUMMARY"
  echo "- Final: PASS" >> "$REPORT"
  exit 0
fi

echo "FAIL count=$FAIL_COUNT" | tee -a "$SUMMARY"
echo "- Final: FAIL ($FAIL_COUNT)" >> "$REPORT"
exit 1
