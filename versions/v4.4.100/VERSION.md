# Version v4.4.100

## Type

Firmware and repository release.

## Firmware Version

The WS63 firmware version is now `v4.4.100`.

## Summary

- Advanced firmware and current repository records from `v4.4.99` to `v4.4.100`.
- Added controlled battery ADC sampling for the SCH_Schematic1_2 v3.2 battery divider.
- Changed the WS63 node heartbeat/hello battery field from a fixed placeholder to an optional firmware battery callback.
- Added `bat status`, `bat sample`, `adc status`, and `adc sample` serial CLI commands.
- Updated build guards to require ADC auto-scan support, WS63 ADC V154, ADC channel 5, and battery ADC status strings.
- Corrected GPS reporting so boards without populated GPS modules are not reported as having GPS hardware.

## Battery ADC

- `ADC_CTRL` -> WS63 `IO05`.
- `ADC_VBAT` -> WS63 `IO12`.
- WS63 ADC channel: `ADC_CHANNEL_5`.
- Schematic divider: R8 = `390k`, R9 = `100k`.
- ADC pin voltage: `ADC_VBAT = VBAT * 100 / (390 + 100)`.
- Firmware conversion: `VBAT_mV = ADC_mV * 490 / 100`.
- Battery percent default curve: `3300mV = 0%`, `4200mV = 100%`, clamped.
- Divider settle time: `50ms`.
- Periodic sample interval: `30s`.

## GPS Status

The tested COM13, COM23, COM24, COM25, and COM26 boards are not populated with GPS modules.
Older logs under `logs/hardware/five_board_relay_loss_20260608_1150/` showed:

- `gps present=1 ready=1 uart=1 tx=17 rx=18 parser=0 module=L80RE-M37`

That line was a firmware false positive: it only proved the optional UART pinmap was configured,
not that a GPS module was physically present. Current firmware leaves GPS disabled by default and
reports `gps configured=0 present=0 ready=0 ... parser=0 module=none` unless a future NMEA detector is added.

## Validation

- Source-level tests assert the v4.4.100 firmware marker, disabled GPS default, ADC channel 5, divider constants, battery percentage thresholds, CLI commands, and heartbeat/hello battery callback path.
- Build scripts guard the v4.4.100 ELF marker plus corrected GPS and battery ADC logging strings.
- Flash and automation defaults now expect `v4.4.100`.
