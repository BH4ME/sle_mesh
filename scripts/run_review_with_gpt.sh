#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

echo "[compat] run_review_with_gpt.sh 已切换为转发入口，默认调用 DeepSeek 审查脚本。"
exec "$SCRIPT_DIR/run_review_with_deepseek.sh" "$@"
