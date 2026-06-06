# Version v4.4.96

## Type

Repository organization release.

## Firmware Version

The WS63 firmware version remains `v4.4.95`. This release does not change firmware behavior or the expected burned firmware package version.

## Summary

- Reorganized root scripts into build, flash, serial, simulation, test, and review categories.
- Added a `hardware/` publication structure for future PCB, schematic, and enclosure uploads.
- Published SLE PCB enclosure `v1.1.4` under hardware version management.
- Rewrote the root README as UTF-8 Chinese-first documentation.
- Added repository layout and version-management documentation.
- Cleaned public documentation indexes that previously had encoding problems.

## Validation

- Script paths updated in current automation and WebUI contract tests.
- Git attributes updated for STEP/STL/PNG/GLB manufacturing files.
- Local draft folders are ignored so future commits stay focused on curated public assets.
