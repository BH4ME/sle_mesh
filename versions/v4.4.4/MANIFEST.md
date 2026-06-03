# v4.4.4 Manifest

## Changed Files

- `README.md`
- `versions/README.md`
- `versions/v4.4.4/VERSION.md`
- `versions/v4.4.4/MANIFEST.md`
- `xc/ws63_team_network/README.md`
- `xc/ws63_team_network/src/ws63_team_network_app.c`
- `xc/ws63_team_network/src/ws63_st7789_display.c`
- `xc/ws63_team_network/sle_uart_client/sle_uart_client.c`
- `webui/tests/ws63-api-contract.test.mjs`

## Key Changes

- Persist leader allowlist in NV so an approved member remains approved after leader reboot.
- Restore the saved allowlist when the leader role is restored or applied.
- Use fixed-profile SLE UART readiness after successful exchange info to avoid property-discovery-only rejoin loops.
- Added contract coverage for leader-reboot recovery behavior.
- Firmware-visible version moved to `v4.4.4`.

## Verification

Pending in this work session:

```sh
npm --prefix webui test
npm --prefix webui run build
git diff --check
```

Firmware build and two-board flash/test will be filled after the remote Ubuntu build and COM16/COM13 validation complete.
