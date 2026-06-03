# Version v4.4.33

Date: 2026-06-02

## Scope

`v4.4.33` is a cleanup and guardrail version after the v4.4.32 pairing fix.
It does not intentionally change SLE networking behavior. Its purpose is to
make the current logic easier to trust by synchronizing version tests and
locking the online/offline ownership boundary with an automated contract test.

## Root Cause Evidence

1. Local C simulations and 30-node Python stress passed, so no new protocol
   behavior bug was reproduced in this pass.
2. WebUI contract tests failed because the version test still expected
   `v4.4.31` while firmware and docs had already moved past that.
3. Manual source review showed member online state has two legitimate writers:
   the core protocol state machine and the WS63 connection-disconnect adapter.
   That boundary should be explicit and automatically checked.

## What Changed

1. Firmware-visible version strings now report `v4.4.33`.
2. README and `versions/README.md` now point to `v4.4.33`.
3. WebUI contract tests now validate the current version.
4. A structural regression test now locks member `online` writes to the
   intended ownership paths.
5. Built the latest firmware package and flashed it to both detected CH340
   boards.

## Verification

Performed locally on this Windows PC:

- `npm test` in `webui/`
- `npm run build` in `webui/`
- `scripts/simulate_v2.sh --suite=all --stress=1`
- `tools/sle_team_python_sim.py --members 30 --ticks 18 --direct-cap 8 --relay-target 3 --batch-fail-relay-count 1 --packet-loss-rate 0.2 --jitter-min-ms 10 --jitter-max-ms 120 --stress 10`

All passed after this version update.

## Firmware Build And Flash

Performed on 2026-06-02 from this Windows PC:

- Build path: remote Ubuntu `owen@192.168.6.5`
- Build fallback: Python Paramiko, because local `sshpass` was not installed
- SDK: `/home/owen/workspace/bearpi-pico_h3863`
- Local package:
  `<repo-root>\output_from_vm\team_network_v4_unified_runtime_role\ws63-liteos-app_v4_unified_all.fwpkg`
- Package size: `1427240`
- Package timestamp: `2026-06-02 17:34:10`
- `COM13`: flashed successfully with manual reset
- `COM16`: flashed successfully with manual reset

Post-flash serial probing on `COM13` and `COM16` did not return `[cfg-json]`
for `cfg status`; only low-level system logs were seen. The flash operation
itself completed successfully on both boards, but runtime CLI version
confirmation still needs a follow-up UART/CLI path check.

## Known Limits

1. Live two-board SLE reconnect testing was not performed in this pass.
2. Runtime serial `cfg status` did not return `[cfg-json]` after flashing, so
   CLI/version readback is not yet confirmed.
3. The new online-state guard is a structural source test; it prevents
   accidental ownership drift, but it is not a substitute for live SLE tests.
