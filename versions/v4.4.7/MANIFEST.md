# v4.4.7 Manifest

## Changed Files

- `README.md`
- `versions/README.md`
- `versions/v4.4.7/VERSION.md`
- `versions/v4.4.7/MANIFEST.md`
- `xc/ws63_team_network/README.md`
- `xc/ws63_team_network/sle_uart_client/sle_uart_client.c`
- `xc/ws63_team_network/src/ws63_team_network_app.c`
- `xc/ws63_team_network/src/ws63_st7789_display.c`
- `scripts/ws63_build_v4_ubuntu.sh`
- `webui/tests/ws63-api-contract.test.mjs`

## Key Changes

- Removed the leader/client `sle_uart_start_scan()` call from the connected
  branch.
- Kept `sle_uart_start_scan()` in the disconnected branch for rediscovery.
- Added a contract test to lock the leader/client reconnect behavior.
- Firmware-visible version moved to `v4.4.7`.

## Verification

To be filled after this board run:

```text
npm --prefix webui test
npm --prefix webui run build
git diff --check
Remote Ubuntu firmware build
COM13 member flash
COM16 leader flash
Two-board member reboot reconnect validation
```
