# V4 Findings

## Requirements

- Main controller: WS63 module.
- Display: 1.14 inch ST7789.
- Main function: SLE team networking.
- Secondary function: when a member is lost, preserve/report the last known state and show the event on the leader side.

## Hardware Mapping Notes

- RGB data: `IO0`
- Charge status input: `IO2`
- Boot: `IO3`
- ADC control: `IO5`
- ST7789 SPI: `SCL/IO6`, `CS/IO7`, `SDA/IO8`, `RS/IO9`, `RESET/IO13`
- Battery ADC: `IO12`
- Buzzer: `IO14`
- GPS UART: `U1TX/IO17`, `U1RX/IO18`
- Debug UART: `U0TX/IO21`, `U0RX/IO22`

## Technical Decisions

- Keep the networking flow as the main line; display is a status and diagnostics surface.
- Keep the WS63 board firmware under `xc/ws63_team_network/` because that matches the SDK sample integration style.
- Use `scripts/build/ws63_build_v4_ubuntu.sh` as the preferred reproducible build entry.
- Enable SoftAP/WebUI by default so runtime role selection is always reachable.
- Enable SPI master for ST7789 display support.
- Treat the project as unified firmware with runtime role selection, not separate leader/member firmware images.

## Validation History

- Local protocol simulations were used for state-machine checks.
- Ubuntu cross-build passed for the V4 line.
- Later v4.4 releases added WebUI, serial runtime config, relay budget, relay failover, and four-board validation records under `versions/`.
