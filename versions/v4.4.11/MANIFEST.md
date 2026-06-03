# v4.4.11 Manifest

## Changed Files (this iteration)

- `xc/ws63_team_network/Kconfig`
- `xc/ws63_team_network/src/ws63_team_network_app.c`
- `xc/ws63_team_network/sle_uart_server/sle_uart_server_adv.c`
- `scripts/ws63_build_v4_ubuntu.sh`
- `README.md`
- `versions/README.md`
- `xc/ws63_team_network/README.md`
- `versions/v4.4.11/VERSION.md`
- `versions/v4.4.11/MANIFEST.md`

## Key Logic Deltas

1. Faster heartbeat/offline policy:
   - `heartbeat_interval_s=1`
   - `heartbeat_timeout_s=4`

2. Faster link supervision:
   - `conn_supervision_timeout=0xFA` (~2500ms)
   - `conn_max_latency=0x20`

3. Leader immediate offline handling on disconnect:
   - map `conn_id -> member_id` in disconnect callback
   - fallback to tracked `route_id` when mapping is missing
   - mark member offline immediately, clear relay flags, clear next-hop routes
   - trigger rescan on downstream disconnect path

4. Rescan cadence tightened:
   - `SLE_TEAM_MEMBER_RESCAN_INTERVAL_S=3`

5. Firmware version bump:
   - `v4.4.11`

## Build / Verification

Remote Ubuntu build: PASS (Python Paramiko fallback)

- Host: `192.168.6.5`
- SDK: `/home/owen/workspace/bearpi-pico_h3863`
- Output package:
  `E:\codex_documents\sle\output_from_vm\team_network_v4_unified_runtime_role\ws63-liteos-app_v4_unified_all.fwpkg`

Package check:

- `strings` contains `v4.4.11` and `v4.4.11 board map`
- Local file size: `1,509,608` bytes

30-node simulation (python model): PASS

- Command:
  `python3 tools/sle_team_python_sim.py --members 30 --direct-cap 8 --relay-fail-tick 6 --relay-recover-tick 10 --ticks 14 --stress 50`
- Result:
  `summary pass=50 fail=0 total=50`
