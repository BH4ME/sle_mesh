# v4.4.66 Flash And Relay Test

## Build

Use the LAN Ubuntu builder, not a local VM:

```powershell
$env:UBUNTU_HOST='192.168.6.5'
$env:UBUNTU_USER='owen'
$env:UBUNTU_PASS='<set locally>'
$env:UBUNTU_SDK='/home/owen/workspace/bearpi-pico_h3863'
$env:BUILD_JOBS='4'
.\.tooling\py311\python.exe .\automation\ws63\tools\ws63_remote_build_v4.py
```

Expected build evidence:

```text
profile:    v4.4.66 unified runtime role
configured v4.4.66 schematic pinmap and approved-member seek admission guard
post-build guard passed
```

## Flash With Visible Progress

Use local Windows for flashing. The script prints each burn line with a COM prefix and writes one log per port.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\ws63_flash_multi.ps1 `
  -Ports COM13,COM17,COM18 `
  -ExpectedVersion v4.4.66 `
  -WaitTimeout 15 `
  -ManualRetryTimeout 300
```

Do not count a board as flashed unless `cfg status` later reports `fw:"v4.4.66"`.

## Three-Board Runtime Test

```powershell
.\.tooling\py311\python.exe .\automation\ws63\tools\ws63_relay_cycle_test.py `
  --leader-port COM13 `
  --relay-port COM17 `
  --child-port COM18 `
  --cfg-runtime-roles `
  --expected-fw v4.4.66 `
  --leader-id 241 `
  --relay-id 224 `
  --child-id 86 `
  --team-id 1 `
  --channel 17 `
  --direct-cap 1 `
  --relay-reboot-command "cfg reboot" `
  --initial-drain-s 1 `
  --bootstrap-timeout-s 30 `
  --cmd-timeout-s 20 `
  --state-timeout-s 70 `
  --parent-timeout-s 70 `
  --relay-offline-timeout-s 25 `
  --relay-boot-timeout-s 100 `
  --failover-timeout-s 100 `
  --poll-interval-s 1
```

Expected evidence:

```text
leader force rescan reason=pairing_window
leader sees member=224
pairing approve member=224 relay=1 ret=0
pairing stop ret=0
relay reboot issued
leader sees member=224 after reboot
route metrics active=2
```

The previous failing shape was repeated `filter:0` for known member `224` / suffix `E7E0` after pairing was closed. That must not recur.
