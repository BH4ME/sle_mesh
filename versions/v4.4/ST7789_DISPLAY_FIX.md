# ST7789 Display Fix

Date: 2026-05-31

## Symptom

The ST7789 screen backlight was on and the panel accepted data, but only part of the screen changed color. The rest showed noisy pixels or a garbled background. Earlier photos showed a central white/yellow block with surrounding noise.

This proved the screen was not burned out: reset, DC, CS, SCLK, and MOSI were all active enough for the panel to receive commands and pixel data.

## Root Cause

The display was treated as `135x240` with offset `52,40`, while this module behaved correctly only with the landscape memory window:

```text
width=240
height=135
x_offset=40
y_offset=53
MADCTL=0x60
```

The visible-area window and MADCTL rotation must match as a set. Changing only orientation or only offset can produce partial correct blocks with noisy surrounding pixels.

## Bad Configuration

This configuration caused the partial-color/noise behavior on the tested module:

```text
width=135
height=240
x_offset=52
y_offset=40
MADCTL=0x00
```

## Working Configuration

The confirmed v4.4 configuration is:

```text
width=240
height=135
x_offset=40
y_offset=53
MADCTL=0x60
soft SPI mode=0
```

Expected startup log:

```text
[display] st7789 ready 240x135 off=40,53 sclk=6 mosi=8 cs=7 dc=9 rst=13
[display] soft-spi enabled mode=0 (cpol=0 cpha=0)
```

## Wiring Reference

- `VCC` to `3.3V`
- `GND` to `GND`
- `SCL/SCK` to `GPIO6`
- `SDA/MOSI` to `GPIO8`
- `CS` to `GPIO7`
- `DC/RS` to `GPIO9`
- `RST/RES` to `GPIO13`
- `BLK` to always-on backlight power/default board wiring

Do not re-add GPIO11 backlight control for this board; backlight is handled outside the firmware.

## Diagnostic Lesson

The useful evidence was the boot log plus the visible partial fill. If future work sees "backlight on, partial fill, noisy rest", first check:

1. `CONFIG_SLE_TEAM_ST7789_WIDTH`
2. `CONFIG_SLE_TEAM_ST7789_HEIGHT`
3. `CONFIG_SLE_TEAM_ST7789_X_OFFSET`
4. `CONFIG_SLE_TEAM_ST7789_Y_OFFSET`
5. `ST7789_MADCTL_DEFAULT`

Do not start by changing SPI speed or rewriting the whole driver; the v4.4 evidence points to the window/rotation tuple as the first suspect.
