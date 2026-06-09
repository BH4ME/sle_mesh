# Version v4.4.119

## Type

Firmware and repository maintenance release.

## Firmware Version

The WS63 firmware version is now `v4.4.119`.

## Summary

- Advanced firmware and current repository records from `v4.4.118` to `v4.4.119`.
- Shared the repeated GPIO output initialization sequence used by the ADC control pin and buzzer forced-off pin path.
- Preserved the GPIO call order, idle levels, ADC sampling setup, muted buzzer safety path, and ST7789 CS-always-low / RS-reset pin-map correction from `v4.4.118`.

## Maintenance Notes

- `team_gpio_config_output_level()` keeps the same mode, pull, pre-output level, output direction, and final level writes as the previous duplicated call sequences.
- ADC control pin setup still applies the ADC off level before `uapi_adc_init(ADC_CLOCK_500KHZ)`.
- Buzzer forced-off setup still validates the pin and applies the configured off level.
- No SLE transport behavior, relay policy, LED/RGB timing, WebUI command syntax, ADC scaling, buzzer logic, or display pin mapping was intentionally changed.
- Historical `v4.4.118` release notes remain unchanged.

## Validation

- Source-level tests should assert the v4.4.119 firmware marker and GPIO initialization helper guard.
- Python syntax checks should cover updated automation tools.
- Remote Ubuntu build should confirm the firmware package contains the `v4.4.119` marker and existing WS63 source guards.
