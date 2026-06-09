# Version v4.4.128

## Type

Firmware hardware bring-up diagnostic and timing release.

## Firmware Version

The WS63 firmware version is now `v4.4.128`.

## Summary

- Advanced firmware and current repository records from `v4.4.127` to `v4.4.128`.
- Reworked the WS2812 bit timing path to use the RISC-V cycle counter during the locked data burst instead of calling `uapi_tcxo_get_count()` for sub-microsecond edges.
- Added a short boot RGB test pattern at level `32` before returning to the dim low-power breathing states.
- Kept normal WS2812 idle, leader, member, and error base colors capped at level `8`.
- Primed the ST7789 CS pin as an output-low before configuring the other display pins, then held RESET high before the normal reset pulse.
- Kept the corrected ST7789 pin map: SCLK GPIO7, MOSI GPIO9, CS GPIO8 held low, RS/DC GPIO13, RESET GPIO10.

## Hardware Notes

- If the display module GND pad is wired to the MCU CS/GPIO8 net, this firmware drives that pin low as early as the display driver starts.
- A GPIO low level is still not a proper power ground. If the panel backlight remains completely dark, the board should be checked for a real GND return and BLK/backlight power.
- If the WS2812 still never shows the short red, green, and blue boot pulses, the next suspects are RGB/IO0 routing, LED power, DIN direction, or a board-level level/timing issue.

## Validation

- Source-level tests assert the v4.4.128 firmware marker, WS2812 cycle-counter timing path, boot RGB diagnostic, ST7789 CS-low priming, and firmware archive-preservation logic.
- Python syntax checks cover updated automation tools.
- Remote Ubuntu build should confirm the firmware package contains the `v4.4.128` marker and new WS2812/ST7789 guard strings.
