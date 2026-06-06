# v4.4.32 Manifest

## Changed Files

- `src/sle_team_node.c`
- `xc/ws63_team_network/src/ws63_team_network_app.c`
- `scripts/ws63_build_v4_ubuntu.sh`
- `README.md`
- `xc/ws63_team_network/README.md`
- `versions/README.md`
- `versions/v4.4.32/VERSION.md`
- `versions/v4.4.32/MANIFEST.md`

## Key Logic Deltas

1. Unknown member HELLO packets now enter pending approval during an open
   pairing window when the allowlist is empty.
2. Empty allowlist remains non-blocking outside that pairing-staging path to
   preserve reboot/rejoin recovery.
3. Firmware-visible version text is now `v4.4.32`.

## Verification

Performed in this iteration:

- Local MSYS2 UCRT64 GCC `16.1.0` compile of C simulations.
- `scripts/simulate_v2.sh --suite=all --stress=1`
- `scripts/simulate_v2.sh --suite=python --stress=10 --py-members=30 --py-direct-cap=8 --py-relay-target=3 --py-fail-tick=6 --py-recover-tick=10 --py-ticks=18 --py-packet-loss-rate=0.2 --py-jitter-min-ms=10 --py-jitter-max-ms=120 --py-batch-fail-relay-count=1 --py-batch-fail-relay-ticks=6`

Not performed in this iteration:

- Firmware flash/hardware runtime test.
