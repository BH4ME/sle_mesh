# v4.4.6 Manifest

## Changed Files

- `README.md`
- `versions/README.md`
- `versions/v4.4.6/VERSION.md`
- `versions/v4.4.6/MANIFEST.md`
- `xc/ws63_team_network/README.md`
- `xc/ws63_team_network/sle_uart_server/sle_uart_server.c`
- `xc/ws63_team_network/src/ws63_team_network_app.c`
- `xc/ws63_team_network/src/ws63_st7789_display.c`
- `scripts/ws63_build_v4_ubuntu.sh`
- `webui/tests/ws63-api-contract.test.mjs`

## Key Changes

- Removed the member/server `sle_uart_server_adv_restart()` call from the connected branch.
- Kept `sle_uart_server_adv_restart()` in the disconnected branch for rediscovery.
- Firmware-visible version moved to `v4.4.6`.

## Verification

Local checks:

```text
npm --prefix webui test       # pass, 41/41
```

Firmware build, package hash, flash and two-board member-reboot validation are recorded here after the board run completes.
