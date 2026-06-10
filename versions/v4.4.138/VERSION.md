# Version v4.4.138

## Type

WS2812C compatibility firmware release.

## Firmware Version

The WS63 firmware version is now `v4.4.138`.

## Summary

- Advanced firmware and repository records from `v4.4.137` to `v4.4.138`.
- Increased the WS2812 reset/latch low interval from `80us` to `320us`.
- Keeps the v4 blink-only RGB behavior from the previous build, without breathing mode.
- Remote build and source guards require `WS63_WS2812_RESET_US 320U`.

## Root Cause Addressed

The board uses WS2812C LEDs. The previous driver left the data line low for only `80us` after sending a frame. WS2812C parts require a longer reset/latch low interval, so later off or blink frames could fail to latch and leave the LED stuck on the previous color, such as solid green after flashing.

## Expected Behavior

- Idle, leader, member, and error states continue to use blink patterns.
- Each new color/off frame should latch reliably on WS2812C.
- The LED should no longer remain stuck solid green just because the previous frame latched but the next frame did not.

## Validation

- Source-level tests cover the firmware version bump and WS2812C reset constant.
- Remote Ubuntu build guards require the final firmware image to contain `v4.4.138`.
- Remote build completed on `192.168.6.5` using `/home/owen/workspace/bearpi-pico_h3863`.
- Firmware package:
  - `output_from_vm/team_network_v4_unified_runtime_role/ws63-liteos-app_v4_unified_all_v4.4.138.fwpkg`
  - size: `1614632` bytes
  - SHA256: `2C68F52EB4F8002351726B4CF45DF29AC9D60320FE7AA4E691EFB9B3321243D6`
- Latest package was also updated:
  - `output_from_vm/team_network_v4_unified_runtime_role/ws63-liteos-app_v4_unified_all.fwpkg`
  - SHA256 matches the archive package above.
