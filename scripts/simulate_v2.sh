#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
LOG_DIR="$ROOT_DIR/logs/sim"
BUILD_DIR="${TMPDIR:-/tmp}/sle_team_sim"
NETWORK_BIN="$BUILD_DIR/sle_team_network_test"
PACKET_BIN="$BUILD_DIR/sle_team_packet_test"
REBALANCE_BIN="$BUILD_DIR/sle_team_relay_rebalance_test"
FAILOVER_BIN="$BUILD_DIR/sle_team_failover_suite_test"
NETWORK_LOG="$LOG_DIR/network_test.log"
PACKET_LOG="$LOG_DIR/packet_test.log"
REBALANCE_LOG="$LOG_DIR/relay_rebalance_test.log"
FAILOVER_LOG="$LOG_DIR/failover_suite.log"

ITERATIONS=1
SUITE="all"
for arg in "$@"; do
  case "$arg" in
    --stress=*)
      ITERATIONS="${arg#*=}"
      ;;
    --suite=*)
      SUITE="${arg#*=}"
      ;;
  esac
done

if ! [[ "$ITERATIONS" =~ ^[0-9]+$ ]] || [ "$ITERATIONS" -lt 1 ]; then
  echo "[sim] ERROR: --stress must be a positive integer, got '$ITERATIONS'" >&2
  exit 1
fi

case "$SUITE" in
  all|core|failover)
    ;;
  *)
    echo "[sim] ERROR: --suite must be one of all|core|failover, got '$SUITE'" >&2
    exit 1
    ;;
esac

mkdir -p "$LOG_DIR" "$BUILD_DIR"

CC_BIN="${CC:-cc}"
if ! command -v "$CC_BIN" >/dev/null 2>&1; then
  if command -v clang >/dev/null 2>&1; then
    CC_BIN="clang"
  elif command -v gcc >/dev/null 2>&1; then
    CC_BIN="gcc"
  else
    echo "[sim] ERROR: no C compiler found (cc/clang/gcc)." >&2
    exit 1
  fi
fi

CFLAGS="-Wall -Werror -I$ROOT_DIR/include"

echo "[sim] using compiler: $CC_BIN"
echo "[sim] build dir: $BUILD_DIR"
echo "[sim] stress iterations: $ITERATIONS"
echo "[sim] suite: $SUITE"

if [ "$SUITE" != "failover" ]; then
  echo "[sim] building network simulation..."
  "$CC_BIN" $CFLAGS \
    "$ROOT_DIR/examples/team_network_demo.c" \
    "$ROOT_DIR/src/sle_team_packet.c" \
    "$ROOT_DIR/src/sle_team_node.c" \
    "$ROOT_DIR/src/sle_team_web_api.c" \
    -DSLE_TEAM_NETWORK_TEST \
    -o "$NETWORK_BIN"

  echo "[sim] building packet simulation..."
  "$CC_BIN" $CFLAGS \
    "$ROOT_DIR/examples/team_node_common.c" \
    "$ROOT_DIR/src/sle_team_packet.c" \
    "$ROOT_DIR/src/sle_team_node.c" \
    -DSLE_TEAM_PACKET_TEST \
    -o "$PACKET_BIN"
fi

if [ "$SUITE" != "core" ]; then
  echo "[sim] building relay rebalance simulation..."
  "$CC_BIN" $CFLAGS \
    "$ROOT_DIR/examples/relay_rebalance_demo.c" \
    -o "$REBALANCE_BIN"
  echo "[sim] building failover suite simulation..."
  "$CC_BIN" $CFLAGS \
    "$ROOT_DIR/examples/relay_failover_suite.c" \
    -o "$FAILOVER_BIN"
fi

PASS_COUNT=0
FAIL_COUNT=0
FAIL_ITERS=""
LAST_ERR=""

: > "$NETWORK_LOG"
: > "$PACKET_LOG"
: > "$REBALANCE_LOG"
: > "$FAILOVER_LOG"

run_once() {
  local i="$1"
  local nlog="$LOG_DIR/network_test.iter${i}.log"
  local plog="$LOG_DIR/packet_test.iter${i}.log"
  local rlog="$LOG_DIR/relay_rebalance_test.iter${i}.log"
  local flog="$LOG_DIR/failover_suite.iter${i}.log"

  if [ "$SUITE" = "core" ]; then
    if "$NETWORK_BIN" > "$nlog" 2>&1 && "$PACKET_BIN" > "$plog" 2>&1; then
      PASS_COUNT=$((PASS_COUNT + 1))
      cat "$nlog" >> "$NETWORK_LOG"
      cat "$plog" >> "$PACKET_LOG"
      return 0
    fi
  elif [ "$SUITE" = "failover" ]; then
    if "$REBALANCE_BIN" > "$rlog" 2>&1 && "$FAILOVER_BIN" > "$flog" 2>&1; then
      PASS_COUNT=$((PASS_COUNT + 1))
      cat "$rlog" >> "$REBALANCE_LOG"
      cat "$flog" >> "$FAILOVER_LOG"
      return 0
    fi
  else
    if "$NETWORK_BIN" > "$nlog" 2>&1 && "$PACKET_BIN" > "$plog" 2>&1 && "$REBALANCE_BIN" > "$rlog" 2>&1 &&
      "$FAILOVER_BIN" > "$flog" 2>&1; then
      PASS_COUNT=$((PASS_COUNT + 1))
      cat "$nlog" >> "$NETWORK_LOG"
      cat "$plog" >> "$PACKET_LOG"
      cat "$rlog" >> "$REBALANCE_LOG"
      cat "$flog" >> "$FAILOVER_LOG"
      return 0
    fi
  fi

  FAIL_COUNT=$((FAIL_COUNT + 1))
  FAIL_ITERS+=" ${i}"
  LAST_ERR="$nlog"
  {
    echo "[sim] iteration $i failed"
    if [ "$SUITE" != "failover" ]; then
      echo "---- network ----"
      cat "$nlog"
      echo "---- packet ----"
      cat "$plog"
    fi
    if [ "$SUITE" != "core" ]; then
      echo "---- relay rebalance ----"
      cat "$rlog"
      echo "---- failover suite ----"
      cat "$flog"
    fi
  } >> "$NETWORK_LOG"
  return 1
}

echo "[sim] running simulations..."
for ((i=1; i<=ITERATIONS; i++)); do
  printf '[sim] iteration %d/%d\n' "$i" "$ITERATIONS"
  run_once "$i" || true
done

echo "[sim] summary: pass=$PASS_COUNT fail=$FAIL_COUNT total=$ITERATIONS"
if [ "$FAIL_COUNT" -ne 0 ]; then
  echo "[sim] failed iterations:$FAIL_ITERS"
  echo "[sim] first failure log: $LAST_ERR"
  exit 1
fi

echo "[sim] done"
if [ "$SUITE" != "failover" ]; then
  echo "[sim] network log: $NETWORK_LOG"
  echo "[sim] packet  log: $PACKET_LOG"
fi
if [ "$SUITE" != "core" ]; then
  echo "[sim] relay   log: $REBALANCE_LOG"
  echo "[sim] failover log: $FAILOVER_LOG"
fi
