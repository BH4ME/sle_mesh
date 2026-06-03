# v4.4.9 Manifest

## Changed Files

- `README.md`
- `versions/README.md`
- `versions/v4.4.9/VERSION.md`
- `versions/v4.4.9/MANIFEST.md`
- `xc/ws63_team_network/README.md`
- `xc/ws63_team_network/sle_uart_client/sle_uart_client.c`
- `xc/ws63_team_network/sle_uart_server/sle_uart_server.c`
- `xc/ws63_team_network/src/ws63_team_network_app.c`
- `xc/ws63_team_network/src/ws63_st7789_display.c`
- `scripts/ws63_build_v4_ubuntu.sh`
- `webui/tests/ws63-api-contract.test.mjs`

## Key Changes

- Deferred client exchange-info until pair-complete unless the link is already
  `SLE_PAIR_PAIRED`.
- Added client pair request return-code logging.
- Added client pair-complete status logging.
- Added server `ssaps_set_info()` return-code logging.
- Firmware-visible version moved to `v4.4.9`.

## Verification

To be filled after this board run:

```text
npm --prefix webui test
npm --prefix webui run build
git diff --check
Remote Ubuntu firmware build
Package string check
COM13 member flash
COM16 leader flash
Two-board member reboot reconnect validation
```
