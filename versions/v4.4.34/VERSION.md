# Version v4.4.34

Date: 2026-06-02

## Scope

`v4.4.34` fixes a flashing/build selection regression found while debugging a
blank ST7789 screen after flashing `v4.4.33`.

## Root Cause Evidence

1. Both boards only printed the SDK SLE UART client/system logs after flashing.
   They did not print `[display] st7789 ready`, `[team] boot unconfigured`,
   `v4.4.33`, or `[cfg-json]`.
2. Remote `.config` had `CONFIG_SAMPLE_SUPPORT_SLE_TEAM_NETWORK` unset while
   `CONFIG_SAMPLE_SUPPORT_SLE_UART_1_VS_8` and
   `CONFIG_SAMPLE_SUPPORT_SLE_UART_CLIENT_1_VS_8` were still enabled.
3. Remote `ws63-liteos-app.map` linked the official `sle_uart_client.c.obj`
   sample but did not link `ws63_team_network_app.c.obj` or
   `ws63_st7789_display.c.obj`.
4. Remote source files were synced correctly, so the failure was sample
   selection/verification, not missing source upload or damaged display
   hardware.

## What Changed

1. Firmware-visible version strings now report `v4.4.34`.
2. The Ubuntu build script now explicitly enables
   `CONFIG_SAMPLE_SUPPORT_SLE_TEAM_NETWORK`.
3. The Ubuntu build script now disables the official SLE UART sample options
   that previously remained selected.
4. The Ubuntu build script now performs a post-build guard:
   it fails if the linked map does not include the team-network app/display
   objects or if the ELF does not contain the expected version/display/config
   strings.
5. ST7789 LVGL initialization now clears the panel before LVGL UI creation and
   forces the first LVGL frame, so a correct firmware shows a boot frame instead
   of clearing the screen black after UI creation.
6. The Ubuntu build script now uses `build.py -c` for a clean app rebuild,
   because the WS63 CMake cache can keep linking the old selected sample even
   after `.config` and `mconfig.h` change.

## Verification

Planned for this iteration after rebuilding:

- `npm test`
- `npm run build`
- `scripts/simulate_v2.sh --suite=all --stress=1`
- 30-node Python stress
- Remote Ubuntu firmware build with the new post-build guard
- Flash `COM13` and `COM16`
- Runtime serial probe for `[display]`, `[team]`, and `[cfg-json]`
