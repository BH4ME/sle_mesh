# Version v4.4

Date: 2026-05-31

## Positioning

`v4.4` is the first confirmed-good display build for the 1.14 inch ST7789 module on the WS63 v4 board. It keeps the v4 runtime-role firmware model and records the remote Ubuntu build plus COM16 auto-flash workflow that worked in the lab.

It also synchronizes the runtime configuration path across firmware business logic, the built-in firmware WebUI, and the external domain WebUI. Bulk node deployment can now be done over serial/WebSerial without connecting each board to WiFi one by one.

## Confirmed Hardware Mapping

- ST7789 SCL/SCLK: `GPIO6`
- ST7789 CS: `GPIO7`
- ST7789 SDA/MOSI: `GPIO8`
- ST7789 DC/RS: `GPIO9`
- ST7789 RESET: `GPIO13`
- ST7789 BLK/backlight: hardware/default-on, no GPIO11 firmware control

## Display Parameters

- Logical size: `240x135`
- X offset: `40`
- Y offset: `53`
- MADCTL: `0x60`
- SPI path: software SPI, mode 0

## Notes

- The earlier `135x240 + off 52,40` setting produced partial-color blocks with surrounding noise on this module.
- The boot color-test diagnostic has been removed after confirmation; normal firmware now returns to the status UI path.
- LVGL remains optional. In the current SDK image, LVGL headers are unavailable, so the built-in text renderer is used.
- Serial config commands are documented in `SERIAL_BULK_CONFIG.md`.
