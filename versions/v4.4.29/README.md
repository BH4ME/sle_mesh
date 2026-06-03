# v4.4.29

## Purpose

Fix the leader SLE seek lifecycle after v4.4.28 failed to rediscover/reconnect members.

## Root Cause Evidence

- Failing logs from `logs/auto_test/v4428_official_seek_basic_20260602_031752/leader.log` show the first `sle_start_seek()` returned `0x0`, then repeated forced rescans returned `0x8000600a` without any `seek result`, `adv_report`, or `will connect`.
- SDK header `/home/owen/workspace/bearpi-pico_h3863/include/middleware/services/bts/sle/sle_errcode.h` defines `ERRCODE_SLE_COMMON_BASE` as `0x80006000` and `ERRCODE_SLE_STATUS_ERR` at offset `0x0A`, so `0x8000600a` is a state error, not a harmless busy result.
- Official sample `/home/owen/workspace/bearpi-pico_h3863/application/samples/products/sle_uart_1_vs_8/sle_uart_client/sle_uart_client.c` starts seek once after SLE enable, stops seek after a matching result, then connects from `seek_disable_cb`. It does not repeatedly call `sle_set_seek_param()` and `sle_start_seek()` while seeking.
- Member advertising data in the failing v4.4.28 log matches the known-good v4.4.12 shape, including `sle_uart_server` scan response, so the primary fault is on the leader seek state machine.

## Code Changes

- `xc/ws63_team_network/sle_uart_client/sle_uart_client.c`
- Skip `sle_uart_start_scan()` when `g_sle_uart_seek_active` is already set.
- Treat only `ERRCODE_SLE_SUCCESS` from `sle_start_seek()` as a successful active seek.
- Remove the incorrect `SLE_UART_SEEK_BUSY_RET` handling for `0x8000600a`.
- Stop clearing `g_sle_uart_seek_active` from connection-count cleanup; seek active state is now owned by seek start/enable/disable callbacks.
- Avoid double-starting scan on the normal `enable_sle()` success path; keep fallback scan only when `enable_sle()` call itself fails, for already-enabled dual-role/relay cases.

## Version

- Firmware version string bumped to `v4.4.29`.

## Verification

- Remote Ubuntu build passed on `192.168.6.5:/home/owen/workspace/bearpi-pico_h3863`.
- Output package copied to `output_from_vm/team_network_v4_unified_runtime_role/ws63-liteos-app_v4_unified_all.fwpkg`.
- Package size: `1427240` bytes.

## Flash Status

- Automatic flashing did not complete on `COM16` or `COM13`.
- Software reset command did not produce a boot handshake.
- DTR/RTS reset sequence on `COM16` also did not produce a boot handshake.
- Next hardware step: enter bootloader manually with reset/BOOT, then rerun the burner for `COM16` and `COM13`.
