# v4.4.3 Manifest

## Changed Files

- `README.md`
- `versions/README.md`
- `versions/v4.4.3/VERSION.md`
- `versions/v4.4.3/MANIFEST.md`
- `xc/ws63_team_network/README.md`
- `xc/ws63_team_network/src/ws63_team_network_app.c`
- `xc/ws63_team_network/src/ws63_st7789_display.c`
- `scripts/ws63_build_v4_ubuntu.sh`
- `scripts/ws63_serial_cfg.ps1`
- `webui/tests/ws63-api-contract.test.mjs`

## Key Changes

- Fixed the PowerShell serial helper runtime failure caused by `$Port` and `$port` being the same variable name in a case-insensitive shell.
- Silenced `Add-Type -AssemblyName System` output inside the helper so function setup cannot pollute the returned serial object.
- Added tests to lock the helper shape: no default DTR/RTS toggle, `Add-Type` is piped to `Out-Null`, and the local serial object uses `$serialPort`.
- Firmware-visible version moved to `v4.4.3`.

## Verification

Local checks:

```sh
npm --prefix webui test            # pass, 38/38
npm --prefix webui run build       # pass
git diff --check                   # pass, only Windows LF->CRLF warnings
```

Firmware build because version strings changed:

```text
Host: 192.168.6.5
User: owen
SDK: /home/owen/workspace/bearpi-pico_h3863
Method: Python paramiko fallback, because this Windows host has ssh but no rsync/sshpass
Result: pass
Display Kconfig: 240x135, offset 40,53 confirmed in remote mconfig.h
```

Package:

```text
<repo-root>\output_from_vm\team_network_v4_unified_runtime_role\ws63-liteos-app_v4_unified_all.fwpkg
Size: 1507176 bytes
Last write: 2026-05-31 13:25:41 +08:00
SHA256: 5C8B737626F7C4D7BF1C323CFC4CCB118B4626E984702C322F0BAD28CEE28860
```

Firmware string check:

```text
v4.4.3: present in package
SLE V4.4.3: present in package
v4.4.3 board map: present in package
v4.4.2 board map: not present
SLE V4.4.2: not present
```

Flash:

```text
Port: COM16
Command: python <repo-root>\automation\ws63\tools\ws63_auto_burn.py -p COM16 -b 115200 --software-reset-only --reset-command reboot --reset-command-fallback reset --reset-command-delay 0.3 --reset-command-retries 2 --reset-command-retry-gap 0.2 <repo-root>\output_from_vm\team_network_v4_unified_runtime_role\ws63-liteos-app_v4_unified_all.fwpkg
Time: 2026-05-31 13:26:13 to 13:28:37 +08:00
Result: pass, all YMODEM transfers reached 100%, tool printed "Done. Reseting device..." and exited 0
```

Runtime serial check:

```text
Command: powershell -ExecutionPolicy Bypass -File scripts\ws63_serial_cfg.ps1 -Port COM16 -Mode status -ReadMs 1800
Result: pass
Reply: [cfg-json] {"ok":true,"fw":"v4.4.3","selfSuffix":"279A","routeId":154,"nvValid":false,"runtimeConfigured":false}
```

Two-board deployment and network check:

```text
Ports: COM16 leader, COM13 member
COM13 flash: pass, all YMODEM transfers reached 100%, tool printed "Done. Reseting device..." and exited 0
Leader: COM16 suffix=279A routeId=154, role=leader, team=1, channel=17
Member: COM13 suffix=E7F1 routeId=241, role=member, leaderSuffix=279A, team=1, channel=17
Pairing: leader received member 241 as pending, pairing approve 241 norelay returned ret=0
Network: leader members showed member=241 online=1; member state showed joined=1
Automation: python automation\ws63\tools\ws63_link_cycle_test.py --leader-port COM16 --member-port COM13 --team-id 1 --leader-id 154 --member-id 241 --channel 17 --cmd-timeout-s 10 --state-timeout-s 25 --poll-interval-s 1 --initial-drain-s 1 --log-dir logs\v4.4.3_link_cycle
Result: pass, connect -> leave -> reconnect
```
