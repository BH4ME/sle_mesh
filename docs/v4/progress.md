# V4 Progress Summary

## Initial V4 Work

- Created the V4 WS63 firmware line from the earlier networking baseline.
- Added WS63 pin mapping for ST7789, WS2812, buzzer, GPS and debug UART.
- Added ST7789 display integration and status/event rendering.
- Kept the networking state machine as the primary behavior.
- Added lost-member and timeout reporting paths.
- Enabled SoftAP/HTTP WebUI and SPI master for board-side operation.
- Verified cross-build through an Ubuntu SDK environment.

## Later v4.4 Work

The detailed v4.4 release history is recorded in [../../versions/README.md](../../versions/README.md).

Recent important records:

- `v4.4.95`: current firmware behavior record.
- `v4.4.96`: repository organization, script layout and hardware-publication structure.

## Current Entry Points

```sh
scripts/build/ws63_build_v4_ubuntu.sh unified
scripts/sim/simulate_v2.sh --suite=python --stress=1
python -m unittest discover -s automation/ws63/tests -t .
```
