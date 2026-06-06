# V4 Task Plan Summary

## Goal

Build the V4 WS63 firmware line with ST7789 display support, runtime leader/member role configuration, and member lost/offline reporting.

## Completed Phases

1. Confirmed hardware mapping and V4 scope.
2. Added V4 board firmware structure under `xc/ws63_team_network/`.
3. Integrated ST7789 display support.
4. Preserved networking as the primary behavior.
5. Added runtime role configuration through serial/WebUI.
6. Added relay budget and relay recovery behavior across later v4.4 versions.
7. Added automated build, flash, simulation and multi-board test tooling.

## Current Follow-Up

The current repository organization work is tracked as `v4.4.96`:

- scripts are split by build/flash/serial/sim/test/review;
- hardware assets are published under `hardware/`;
- root README and public indexes are UTF-8 Chinese-capable.
