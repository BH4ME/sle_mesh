# v4.4.8 Manifest

## Changed Files

- `README.md`
- `versions/README.md`
- `versions/v4.4.8/VERSION.md`
- `versions/v4.4.8/MANIFEST.md`
- `xc/ws63_team_network/README.md`
- `xc/ws63_team_network/sle_uart_client/sle_uart_client.c`
- `xc/ws63_team_network/src/ws63_team_network_app.c`
- `xc/ws63_team_network/src/ws63_st7789_display.c`
- `scripts/ws63_build_v4_ubuntu.sh`
- `webui/tests/ws63-api-contract.test.mjs`

## Key Changes

- Added per-connection `exchange_requested` state.
- Replaced direct pair-complete exchange-info with
  `sle_uart_client_exchange_once(conn_id, "pair-complete")`.
- Added a contract test that forbids raw `ssapc_exchange_info_req()` in
  pair-complete.
- Firmware-visible version moved to `v4.4.8`.

## Verification

```text
npm --prefix webui test: PASS, 43/43
npm --prefix webui run build: PASS
git diff --check: PASS, Windows LF->CRLF warnings only
Remote Ubuntu firmware build: PASS via Python Paramiko fallback
Package string check: PASS
  size: 1508520 bytes
  contains: v4.4.8, SLE V4.4.8, v4.4.8 board map, exchange info already requested
  does not contain: v4.4.7 / SLE V4.4.7
COM13 member flash: PASS
COM16 leader flash: PASS
Two-board member reboot reconnect validation: FAIL
```

Failure evidence:

```text
leader before reboot: member=241 online=1
member before reboot: state=3 joined=1
leader after member reboot: repeated local disconnect 0x11
member after reboot: repeated remote disconnect 0x10, joined=0
```
