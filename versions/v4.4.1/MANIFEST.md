# v4.4.1 Manifest

## Changed Files

- `README.md`
- `meta/DOC_WORKFLOW.md`
- `meta/PROJECT_OPERATION_SOP.md`
- `versions/README.md`
- `versions/v4.4.1/VERSION.md`
- `versions/v4.4.1/MANIFEST.md`
- `xc/ws63_team_network/README.md`
- `xc/ws63_team_network/Kconfig`
- `xc/ws63_team_network/src/ws63_team_network_app.c`
- `xc/ws63_team_network/src/ws63_st7789_display.c`
- `scripts/ws63_build_v4_ubuntu.sh`
- `webui/tests/ws63-api-contract.test.mjs`

## Key Changes

- Added a mandatory project operation SOP for future code changes, remote builds, auto flashing, version management, rollback, and hardware pitfalls.
- Bumped firmware-visible version from `v4.4` to `v4.4.1`.
- Kept the confirmed `v4.4` screen and serial deployment behavior unchanged.
- Added tests to make the current firmware version and SOP entry points harder to forget.

## Verification

Local checks:

```sh
npm --prefix webui test            # pass, 35/35
npm --prefix webui run build       # pass
git diff --check                   # pass, only Windows LF->CRLF warnings
```

Firmware build because version strings changed:

```sh
Host: 192.168.6.5
User: owen
SDK: /home/owen/workspace/bearpi-pico_h3863
Method: Python paramiko fallback, because this Windows host has ssh but no rsync/sshpass
Result: pass
```

Package:

```text
<repo-root>\output_from_vm\team_network_v4_unified_runtime_role\ws63-liteos-app_v4_unified_all.fwpkg
Size: 1507176 bytes
Last write: 2026-05-31 01:25:02 +08:00
SHA256: 13F461F124552BC8DC51FCF06EE48E4E5C44F0D94B2BF54B0FE25B6673F817D6
```

Firmware string check:

```text
v4.4.1: present in package
SLE V4.4.1: present in package
v4.4.1 board map: present in package
v4.4 board map: not present
```

Flash:

```text
Flash result: not requested for this SOP patch
```
