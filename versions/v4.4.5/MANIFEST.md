# v4.4.5 Manifest

## Changed Files

- `README.md`
- `versions/README.md`
- `versions/v4.4.5/VERSION.md`
- `versions/v4.4.5/MANIFEST.md`
- `xc/ws63_team_network/README.md`
- `xc/ws63_team_network/src/ws63_team_network_app.c`
- `xc/ws63_team_network/src/ws63_st7789_display.c`
- `scripts/ws63_build_v4_ubuntu.sh`
- `scripts/ws63_serial_cfg.ps1`
- `webui/tests/ws63-api-contract.test.mjs`

## Key Changes

- Do not let a non-success `uapi_nv_flush()` overwrite a successful `uapi_nv_write()` result.
- Keep flush result visible in logs for future diagnostics.
- Add serial helper `apply` mode.
- Firmware-visible version moved to `v4.4.5`.

## Verification

Local checks:

```sh
npm --prefix webui test       # pass, 40/40
npm --prefix webui run build  # pass
git diff --check              # pass, only Windows LF->CRLF warnings
```

Firmware build:

```text
Host: 192.168.6.5
User: owen
SDK: /home/owen/workspace/bearpi-pico_h3863
Method: Python paramiko fallback, because this Windows host has ssh but no rsync/sshpass
Result: pass
Memory summary: SRAM 196208 / 548608 bytes (35.76%), PROGRAM 1347580 / 2357504 bytes (57.16%)
```

Package:

```text
<repo-root>\output_from_vm\team_network_v4_unified_runtime_role\ws63-liteos-app_v4_unified_all.fwpkg
Size: 1508136 bytes
SHA256: E31395BBE43A4ED203B27A317F089ECD1CD70279DDF8600F2F7B148E27057AFB
String check: v4.4.5 / SLE V4.4.5 / v4.4.5 board map present; v4.4.4 absent
```

Flash:

```text
COM13: pass, software reset entered download mode and all YMODEM transfers reached 100%
COM16: pass, required manual RESET during the tool's manual retry window; all YMODEM transfers reached 100%
```

Two-board runtime validation:

```text
COM16: leader, suffix 279A, routeId 154
COM13: member, suffix E7F1, routeId 241
Config commands: cfg leader now 1 17, cfg member now 279A 1 17
Config result: ret=0 on both boards; uapi_nv_write ret=0x0 and uapi_nv_flush warning=0x80000002 logged but non-fatal
Pairing: pairing start -> pending member 241 -> pairing approve 241 norelay ret=0
Before leader reboot: leader members showed member=241 online=1; member state showed joined=1
Leader-only reboot: cfg reboot on COM16
After leader reboot: COM16 restored leader runtime from NV and received member heartbeat; COM16 members showed member=241 online=1; COM13 state showed joined=1
```
