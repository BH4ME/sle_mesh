# v4.4.33 Manifest

## Changed Files

- `README.md`
- `versions/README.md`
- `versions/v4.4.33/VERSION.md`
- `versions/v4.4.33/MANIFEST.md`
- `webui/tests/ws63-api-contract.test.mjs`
- `scripts/ws63_build_v4_ubuntu.sh`
- `xc/ws63_team_network/src/ws63_team_network_app.c`

## Key Logic Deltas

1. No intended firmware networking behavior change.
2. Firmware-visible version text is now `v4.4.33`.
3. Contract tests now protect the member online/offline state ownership
   boundary between `src/sle_team_node.c` and the WS63 adapter.
4. Build-script visible profile text now reports `v4.4.33`.

## Verification

Performed in this iteration:

- `npm test`
- `npm run build`
- Local MSYS2 UCRT64 GCC compile of C simulations.
- `scripts/simulate_v2.sh --suite=all --stress=1`
- `tools/sle_team_python_sim.py --members 30 --ticks 18 --direct-cap 8 --relay-target 3 --batch-fail-relay-count 1 --packet-loss-rate 0.2 --jitter-min-ms 10 --jitter-max-ms 120 --stress 10`
- Remote Ubuntu firmware build via Python Paramiko fallback.
- Flash `COM13` with manual reset.
- Flash `COM16` with manual reset.

Not performed in this iteration:

- Live two-board SLE reconnect test.
- Runtime CLI `[cfg-json]` version readback was not confirmed after flashing.
