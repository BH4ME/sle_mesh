# v4.4.75 Manifest

Date: 2026-06-05

## Files Updated

```text
xc/ws63_team_network/src/ws63_team_network_app.c
xc/ws63_team_network/src/ws63_st7789_display.c
automation/ws63/tools/ws63_remote_build_v4.py
automation/ws63/tools/ws63_auto_burn.py
automation/ws63/tests/test_ws63_auto_burn.py
automation/ws63/tests/test_ws63_four_board_relay_test.py
scripts/ws63_flash_multi.ps1
README.md
versions/README.md
meta/PROJECT_OPERATION_SOP.md
```

## Files Added

```text
versions/v4.4.75/VERSION.md
versions/v4.4.75/MANIFEST.md
```

## Firmware

Expected output package:

```text
<repo-root>\output_from_vm\team_network_v4_unified_runtime_role\ws63-liteos-app_v4_unified_all.fwpkg
```

Required firmware string:

```text
v4.4.75
```

## Validation Plan

Local tests:

```powershell
.\.tooling\py311\python.exe -m unittest `
  automation.ws63.tests.test_ws63_four_board_relay_test `
  automation.ws63.tests.test_ws63_auto_burn
```

Remote build:

```powershell
$env:UBUNTU_HOST='192.168.6.5'
$env:UBUNTU_USER='owen'
$env:UBUNTU_PASS='<set locally, do not commit secrets>'
$env:UBUNTU_SDK='/home/owen/workspace/bearpi-pico_h3863'
$env:BUILD_JOBS='4'
.\.tooling\py311\python.exe .\automation\ws63\tools\ws63_remote_build_v4.py `
  --host 192.168.6.5 `
  --user owen `
  --password <set-locally> `
  --sdk /home/owen/workspace/bearpi-pico_h3863 `
  --jobs 4
```

Parallel software-reset flash:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\ws63_flash_multi.ps1 `
  -Ports COM16,COM13,COM17,COM18 `
  -ExpectedVersion v4.4.75 `
  -Parallel `
  -ParallelStartDelayMs 1200 `
  -WaitTimeout 45 `
  -ManualRetryTimeout 0
```

Four-board relay test:

```powershell
.\.tooling\py311\python.exe .\automation\ws63\tools\ws63_four_board_relay_test.py `
  --leader-port COM16 `
  --relay-port COM13 `
  --child1-port COM17 `
  --child2-port COM18 `
  --expected-fw v4.4.75 `
  --team-id 1 `
  --channel 17 `
  --direct-cap 1 `
  --reboot-command "cfg reboot" `
  --initial-drain-s 1 `
  --cmd-timeout-s 25 `
  --state-timeout-s 90 `
  --route-timeout-s 120 `
  --offline-timeout-s 20 `
  --boot-timeout-s 45 `
  --failover-timeout-s 120 `
  --poll-interval-s 1 `
  --log-dir logs\live\v4.4.75_four_board_com16_leader_<timestamp>
```

## Notes

The reliable flash process is software reset only. Do not ask for manual reset
unless the board does not answer serial commands at all.
