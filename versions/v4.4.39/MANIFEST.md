# v4.4.39 Manifest

## Changed Files

- `README.md`
- `versions/README.md`
- `versions/v4.4.39/VERSION.md`
- `versions/v4.4.39/MANIFEST.md`
- `examples/team_node_common.c`
- `examples/team_network_demo.c`
- `examples/relay_rebalance_demo.c`
- `examples/relay_failover_suite.c`
- `webui/tests/ws63-api-contract.test.mjs`

## Key Deltas

1. Replaced direct casts from decoded packet body byte buffers to aligned structs in C demos with local `memcpy` decode copies.
2. Added RSSI seed helpers in relay demos so member lookup results are checked before dereference.
3. Updated repository version pointers from `v4.4.38` to `v4.4.39`.

## Verification

Completed with local WSL GCC:

- Strict C demo compile/run for packet, network, regression, relay rebalance, relay failover, and terminal examples.
- ASAN/UBSAN C demo compile/run for the same examples.
- GCC `-fanalyzer` C demo compile/run for the same examples.
- `git diff --check`: pass with only Windows LF-to-CRLF conversion warnings.
- `npm --prefix webui test`: 53/53 pass.
