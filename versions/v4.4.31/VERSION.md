# Version v4.4.31

Date: 2026-06-02

## Scope

`v4.4.31` fixes the deployment path where the hosted/domain WebUI depends on the
board HTTP API, but the firmware build could leave SoftAP/HTTP WebUI from
auto-starting.

## Root Cause Evidence

1. `team_wifi_ap_task()` reaches `team_http_server_loop()` only after
   `CONFIG_SLE_TEAM_WIFI_AP_AUTO_START` is true.
2. The previous fallback/default path could leave
   `CONFIG_SLE_TEAM_WIFI_AP_AUTO_START` unset or false, so WiFi init completed
   but the task returned before TCP/IP wait, SoftAP start, and HTTP server start.
3. The Ubuntu build script enabled `CONFIG_SLE_TEAM_WIFI_AP_ENABLE=y`, but did
   not explicitly set `CONFIG_SLE_TEAM_WIFI_AP_AUTO_START=y`, so remote `.config`
   state could drift from the source intent.
4. The hosted WebUI also uses WebSerial for bulk node configuration. Its serial
   command runner reacquired a reader per command and raced reads against a
   timeout, which could drop `[cfg-json]` lines during batch configuration.

## What Changed

1. Firmware fallback now keeps `CONFIG_SLE_TEAM_WIFI_AP_AUTO_START` enabled by
   default.
2. Kconfig now defines `SLE_TEAM_WIFI_AP_AUTO_START` with `default y` and
   documents that the hosted/domain WebUI still needs the board HTTP API.
3. The remote Ubuntu build script explicitly writes
   `CONFIG_SLE_TEAM_WIFI_AP_AUTO_START=y`, LVGL enable, and LVGL draw buffer
   values into the SDK `.config`.
4. WebSerial now uses one background reader per selected port and command calls
   read new lines from the shared serial log instead of racing and releasing
   pending reads.
5. Firmware-visible version strings and project docs are synchronized to
   `v4.4.31`.

## Known Limits

1. This version does not change the SLE mesh/member rejoin logic beyond the
   already-present `v4.4.30` fixes.
2. Hardware flashing and live board WebUI verification still require a separate
   on-device run after this compile is burned.
