# v4.4.2 Manifest

## Changed Files

- `README.md`
- `versions/README.md`
- `versions/v4.4.2/VERSION.md`
- `versions/v4.4.2/MANIFEST.md`
- `xc/ws63_team_network/README.md`
- `xc/ws63_team_network/src/ws63_team_network_app.c`
- `xc/ws63_team_network/src/ws63_st7789_display.c`
- `scripts/ws63_build_v4_ubuntu.sh`
- `scripts/ws63_serial_cfg.ps1`
- `webui/src/main.ts`
- `webui/tests/ws63-api-contract.test.mjs`

## Key Changes

- Added contract coverage for three deployment bugs before fixing them.
- WebUI now rejects failed config replies instead of showing them as successful writes.
- WebUI bulk config form now uses nullish fallback so valid `0` values survive.
- Serial config helper now leaves DTR/RTS low/off unless `-UseControlLines` is explicitly requested.
- Serial config helper avoids the PowerShell `$Port`/`$port` case-insensitive variable collision so COM port name and serial object do not overwrite each other.
- Firmware-visible version moved to `v4.4.2`.

## Verification

Local checks:

```sh
npm --prefix webui test            # pass, 38/38
npm --prefix webui run build       # pass
git diff --check                   # pass, only Windows LF->CRLF warnings
powershell -ExecutionPolicy Bypass -File scripts\ws63_serial_cfg.ps1 -Port COM16 -Mode status -ReadMs 1800
# pass, board replied with [cfg-json] fw="v4.4.2", selfSuffix="279A", nvValid=false, runtimeConfigured=false
```

Firmware build because version strings changed:

```text
Host: 192.168.6.5
User: owen
SDK: /home/owen/workspace/bearpi-pico_h3863
Method: Python paramiko fallback, because this Windows host has ssh but no rsync/sshpass
Result: pass
Display Kconfig: 240x135, offset 40,53 confirmed in remote mconfig.h
Memory: SRAM 196208/548608 bytes (35.76%), PROGRAM 1346620/2357504 bytes (57.12%)
```

Package:

```text
E:\codex_documents\sle\output_from_vm\team_network_v4_unified_runtime_role\ws63-liteos-app_v4_unified_all.fwpkg
Size: 1507176 bytes
Last write: 2026-05-31 01:44:00 +08:00
SHA256: AA31C7A24568EA27EBBB50F037AF2A9D4618661F2BE710512A832406EBEDCDF8
```

Firmware string check:

```text
v4.4.2: present in package
SLE V4.4.2: present in package
v4.4.2 board map: present in package
v4.4.1 board map: not present
SLE V4.4.1: not present
```

Flash:

```text
Port: COM16
Command: python E:\codex_documents\sle\automation\ws63\tools\ws63_auto_burn.py -p COM16 -b 115200 --software-reset-only --reset-command reboot --reset-command-fallback reset --reset-command-delay 0.3 --reset-command-retries 2 --reset-command-retry-gap 0.2 E:\codex_documents\sle\output_from_vm\team_network_v4_unified_runtime_role\ws63-liteos-app_v4_unified_all.fwpkg
Time: 2026-05-31 12:58:55 to 13:01:19 +08:00
Result: pass, all YMODEM transfers reached 100%, tool printed "Done. Reseting device..." and exited 0
```
