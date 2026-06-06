# Version v4.4.15

Date: 2026-06-01

## Scope

`v4.4.15` completes LVGL integration for the WS63 ST7789 runtime UI by adding
a project-local LVGL configuration that is compatible with the WS63 `-Werror`
toolchain, while keeping the UI focused on status labels.

## Root Cause

LVGL source was present, but the default config implicitly enabled many extra
widgets (including `span`) that triggered `-Werror=jump-misses-init` in this
SDK/compiler combination, causing firmware build failure before packaging.

## What Changed

1. Firmware version bump:
   - `SLE_TEAM_FW_VERSION`: `v4.4.14 -> v4.4.15`
   - `SLE_TEAM_HW_CONSTRAINTS`: `v4.4.14 board map -> v4.4.15 board map`

2. LVGL config hardening for WS63:
   - Updated `third_party/lvgl/lv_conf.h` to keep a minimal, label-oriented UI.
   - Disabled unused core widgets and extra widgets/themes/layouts.
   - Retained only required draw/text/font capabilities for current screen UI.

3. Documentation/version index sync:
   - Updated root and module README current-version markers.
   - Added this version record and manifest entry.

## Expected Impact

- LVGL backend can compile and link on the remote Ubuntu WS63 SDK build path.
- Firmware still uses the same status/alert screen semantics, with LVGL label
  rendering path available at runtime.
- Lower compile risk and footprint by avoiding unused LVGL modules.
