# v4.4 Manifest

## Changed Files

- `xc/ws63_team_network/src/ws63_st7789_display.c`
- `xc/ws63_team_network/src/ws63_team_network_app.c`
- `xc/ws63_team_network/Kconfig`
- `xc/ws63_team_network/sle_uart_server/sle_uart_server_adv.c`
- `webui/src/api/client.ts`
- `webui/src/main.ts`
- `webui/src/protocol/types.ts`
- `webui/src/styles/app.css`
- `webui/shared/ws63-api.json`
- `webui/tests/ws63-api-contract.test.mjs`
- `scripts/ws63_serial_cfg.ps1`
- `scripts/ws63_build_v4_ubuntu.sh`
- `versions/README.md`
- `versions/v4.4/VERSION.md`
- `versions/v4.4/MANIFEST.md`
- `versions/v4.4/AUTO_FLASH_NOTES.md`
- `versions/v4.4/ST7789_DISPLAY_FIX.md`
- `versions/v4.4/SERIAL_BULK_CONFIG.md`

## Key Changes

- Locked the confirmed ST7789 configuration to `240x135`, offset `40,53`, MADCTL `0x60`.
- Synchronized the same display tuple across fallback defines, `Kconfig`, and the remote Ubuntu build script.
- Removed the temporary boot color-test loop after the screen was confirmed working.
- Added unified device config over firmware serial CLI and HTTP `/api/config/*`.
- Added external WebUI one-click node configuration with WebSerial log display.
- Added a PowerShell serial config helper for fleet deployment.
- Unified SLE advertise TX power value and scan-response declaration at `18 dBm`.
- Documented the successful remote Ubuntu build and `COM16` auto-flash process.
- Documented the display symptom, root cause, bad configuration, and final working configuration.
- Documented the serial bulk configuration workflow.

## Verification

Remote Ubuntu build:

```sh
UBUNTU_HOST=192.168.6.5 \
UBUNTU_USER=owen \
UBUNTU_PASS='<set locally, do not commit secrets>' \
UBUNTU_SDK=/home/owen/workspace/bearpi-pico_h3863 \
BUILD_JOBS=4 \
scripts/ws63_build_v4_ubuntu.sh unified
```

Expected boot log after flashing:

```text
[display] st7789 ready 240x135 off=40,53 sclk=6 mosi=8 cs=7 dc=9 rst=13
[display] soft-spi enabled mode=0 (cpol=0 cpha=0)
```

Local checks:

```sh
npm --prefix webui test
npm --prefix webui run build
git diff --check
```

Remote `mconfig.h` confirmation:

```text
#define CONFIG_SLE_TEAM_WS2812_ENABLE 1
#define CONFIG_SLE_TEAM_ST7789_X_OFFSET 40
#define CONFIG_SLE_TEAM_ST7789_Y_OFFSET 53
#define CONFIG_SLE_TEAM_ST7789_WIDTH 240
#define CONFIG_SLE_TEAM_ST7789_HEIGHT 135
```
