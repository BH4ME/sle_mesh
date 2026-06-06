# v4.4.30 Manifest

## Changed Files

- `src/sle_team_node.c`
- `examples/team_node_regression_test.c`
- `xc/ws63_team_network/sle_uart_client/sle_uart_client.c`
- `xc/ws63_team_network/sle_uart_server/sle_uart_server_adv.c`
- `xc/ws63_team_network/sle_uart_server/sle_uart_server_adv.h`
- `xc/ws63_team_network/src/ws63_team_network_app.c`
- `webui/tests/ws63-api-contract.test.mjs`
- `README.md`
- `versions/README.md`
- `xc/ws63_team_network/README.md`
- `versions/v4.4.30/VERSION.md`
- `versions/v4.4.30/MANIFEST.md`

## Key Logic Deltas

1. Offline member rejoin reuses the original logical member slot instead of
   creating duplicate offline records.
2. Public `sle_team_node_find_member()` keeps its online-only behavior, while
   internal logical-record lookup now supports offline member reuse safely.
3. SLE client discovery/write callbacks no longer dereference failed SDK data.
4. Relay dual-role callback registration now merges announce and seek callback
   fields before registering with the SDK.
5. Relay RSSI stale-link recovery is keyed off connected count, not only
   discovery-ready state.

## Verification

Performed in this iteration:

- `npm --prefix webui test`
- `npm --prefix webui run build`
- Remote Ubuntu firmware compile through the SOP fallback path:
  Python `paramiko` sync/SSH on `owen@192.168.6.5`, SDK
  `/home/owen/workspace/bearpi-pico_h3863`, then
  `python3 build.py ws63-liteos-app -j4`.
- Firmware package downloaded to
  `output_from_vm/team_network_v4_unified_runtime_role/ws63-liteos-app_v4_unified_all.fwpkg`
  with size `1427240` bytes (`1.36 MiB`).
- `git diff --check -- src/sle_team_node.c examples/team_node_regression_test.c xc/ws63_team_network/sle_uart_client/sle_uart_client.c xc/ws63_team_network/sle_uart_server/sle_uart_server_adv.c xc/ws63_team_network/sle_uart_server/sle_uart_server_adv.h xc/ws63_team_network/src/ws63_team_network_app.c webui/tests/ws63-api-contract.test.mjs README.md versions/README.md xc/ws63_team_network/README.md versions/v4.4.30/VERSION.md versions/v4.4.30/MANIFEST.md`

Not performed in this iteration:

- Local C regression build, because no local C compiler was available in this
  environment.
- Firmware flash/hardware runtime test was not performed in this compile-only
  turn.
