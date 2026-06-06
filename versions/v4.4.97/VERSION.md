# Version v4.4.97

## Type

Repository cleanup release.

## Firmware Version

The WS63 firmware version remains `v4.4.95`. This release removes redundant repository files and does not change firmware behavior.

## Summary

- Removed the tracked `vm-preseed/` installer boot files and weak-password preseed config from the public repository.
- Removed generated/stale review output `meta/review_feedback.md`; future review output stays local and ignored.
- Removed unreferenced planning/candidate notes from `docs/v4/`, `docs/v2/`, and `meta/`.
- Moved the V4 schematic PDF from `docs/v4/reference/` into `hardware/schematics/`.
- Updated documentation to describe generated outputs and local-only VM/preseed artifacts as ignored files.
- Kept core firmware, automation, WebUI, hardware enclosure assets and version records intact.

## Validation

- Repository size audit identified `vm-preseed/` as the only large unnecessary tracked directory.
- The retained hardware enclosure files remain under `hardware/enclosures/sle-pcb-enclosure/v1.1.4/`.
- Focused test, WebUI build, simulation, and remote WS63 firmware build checks are recorded in `MANIFEST.md`.
