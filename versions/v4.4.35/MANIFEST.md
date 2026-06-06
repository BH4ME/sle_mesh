# v4.4.35 Manifest

## Changed Files

- `README.md`
- `versions/README.md`
- `versions/v4.4.35/VERSION.md`
- `versions/v4.4.35/MANIFEST.md`
- `webui/src/api/client.ts`
- `webui/tests/ws63-api-contract.test.mjs`
- `scripts/ws63_build_v4_ubuntu.sh`
- `xc/ws63_team_network/src/ws63_team_network_app.c`

## Key Logic Deltas

1. Unifies old role shortcuts with the new one-click config path.
2. Keeps `/api/role` only as a compatibility route and makes its leader default
   path safe while the board is still unconfigured.
3. Preserves the `v4.4.34` build-selection guard, ST7789/LVGL boot-frame guard,
   and 30-node/rejoin logic.

## Verification

- PASS: `npm test` in `webui` (`51/51`).
- PASS: `npm run build` in `webui`.
- PASS: Remote Ubuntu clean firmware build with Paramiko fallback.
- PASS: Remote build guard verified `.config`, `.map`, and `.elf` include the
  expected `v4.4.35` team-network/display/config symbols and exclude the
  official SLE UART sample.
- BLOCKED locally: `scripts/simulate_v2.sh --suite=all --stress=1` needs a
  local C compiler (`cc`, `clang`, or `gcc`), which is not installed here.
