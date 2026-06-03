# v4.4.34 Manifest

## Changed Files

- `README.md`
- `versions/README.md`
- `versions/v4.4.34/VERSION.md`
- `versions/v4.4.34/MANIFEST.md`
- `webui/tests/ws63-api-contract.test.mjs`
- `scripts/ws63_build_v4_ubuntu.sh`
- `xc/ws63_team_network/src/ws63_team_network_app.c`
- `xc/ws63_team_network/src/ws63_st7789_display.c`

## Key Logic Deltas

1. Fixes the root cause of the blank-screen flash: the remote build was still
   selecting the official SLE UART 1-vs-8 sample instead of `sle_team_network`.
2. Adds a hard post-build guard so a wrong sample cannot silently produce a
   "successful" firmware package.
3. Adds a first-frame LVGL display guard so the ST7789 is not left black after
   LVGL UI creation.
4. Forces a clean WS63 app build so CMake cannot keep stale sample selection
   from a previous official UART demo build.

## Verification

To be completed after this build/flash pass:

- Local WebUI tests and build.
- Local protocol simulations.
- Remote Ubuntu firmware build with map/ELF guard.
- Flash both CH340 boards and confirm runtime logs.
