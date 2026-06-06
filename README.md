# sle_mesh

`sle_mesh` is a WS63 SLE mesh networking project with runtime leader/member role configuration, relay failover, a board-side HTTP UI, and a browser WebUI.

## Current Release

- Repository record: `v4.4.95`
- Firmware version: `v4.4.95`
- Version notes: [versions/v4.4.95/VERSION.md](versions/v4.4.95/VERSION.md)
- Release index: [versions/README.md](versions/README.md)

## What Is Included

- Shared protocol headers and logic in [include/](include) and [src/](src)
- Local examples and regression programs in [examples/](examples)
- WS63 board firmware under [xc/ws63_team_network/](xc/ws63_team_network)
- Browser WebUI under [webui/](webui)
- Build, flash, serial configuration, and validation helpers under [scripts/](scripts)
- WS63 automation tools under [automation/ws63/](automation/ws63)
- Version and hardware validation records under [versions/](versions)

## Documentation

- [docs/README.md](docs/README.md): documentation index
- [docs/branch_strategy.md](docs/branch_strategy.md): branch roles and merge policy
- [docs/v4/README.md](docs/v4/README.md): current WS63/ST7789 line
- [meta/PROJECT_OPERATION_SOP.md](meta/PROJECT_OPERATION_SOP.md): operating SOP
- [meta/DOC_WORKFLOW.md](meta/DOC_WORKFLOW.md): documentation workflow

## v4.4.95 Highlights

- Preserves already-online members when a leader pairing window is closed.
- Keeps the dynamic relay budget introduced in the v4.4 line.
- Keeps relay swap hysteresis: a non-relay candidate must stay at least 8 dB stronger than the weakest active relay for 30 seconds before swapping.
- Supports natural four-board validation without forcing `cfg direct`.
- Uses one unified firmware package for all WS63 nodes; leader/member roles are selected at runtime through serial commands or the WebUI.

## Board WebUI

Default board-side Wi-Fi:

```text
SSID: SLE-TEAM-V4-XXXX
Password: 123456789
URL: http://192.168.43.1/
```

Useful pages:

- `/`: status view
- `/nodes`: joined nodes
- `/events`: recent send/receive events
- `/pairing`: role selection, pairing, leader selection, and phone location

Useful API endpoints:

- `GET /api/status`
- `GET /api/nodes`
- `GET /api/events`
- `GET /api/pending`
- `GET /api/location?lat=...&lon=...&dst=255&speed=...&heading=...&battery=...&fix=...&sat=...`
- `GET /api/config/status`
- `GET /api/config/leader?team=1&channel=17&now=1`
- `GET /api/config/member?leader=C7E9&team=1&channel=17&now=1`
- `GET /api/config/apply`
- `GET /api/config/clear`
- `GET /api/config/reboot`
- `GET /api/pairing?action=start|stop|approve&id=...&relay=0|1`
- `GET /api/member/select?team=...&leader=...&channel=...`
- `GET /api/member/leave`
- `GET /api/factory-reset`

## Serial Configuration

Common serial commands:

```text
cfg status
cfg leader now <team> <channel>
cfg member now <leader_suffix_hex> <team> <channel>
cfg apply
cfg clear
cfg reboot
```

Windows PowerShell helper:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/ws63_serial_cfg.ps1 -Port COM7 -Mode leader -Team 7 -Channel 33
powershell -ExecutionPolicy Bypass -File scripts/ws63_serial_cfg.ps1 -Port COM8 -Mode member -LeaderSuffix 9A2F -Team 7 -Channel 33
powershell -ExecutionPolicy Bypass -File scripts/ws63_serial_cfg.ps1 -Port COM8 -Mode status
```

## Remote Build

Preferred build path uses the LAN Ubuntu build host:

```sh
UBUNTU_HOST=192.168.6.5 \
UBUNTU_USER=owen \
UBUNTU_PASS='<set locally, do not commit secrets>' \
UBUNTU_SDK=/home/owen/workspace/bearpi-pico_h3863 \
BUILD_JOBS=4 \
scripts/ws63_build_v4_ubuntu.sh unified
```

Unified firmware output:

```text
<repo-root>\output_from_vm\team_network_v4_unified_runtime_role\ws63-liteos-app_v4_unified_all.fwpkg
```

## Flash Example

```powershell
python <repo-root>\automation\ws63\tools\ws63_auto_burn.py `
  -p COM16 `
  -b 115200 `
  --software-reset-only `
  --reset-command reboot `
  --reset-command-fallback reset `
  --reset-command-delay 0.3 `
  --reset-command-retries 2 `
  --reset-command-retry-gap 0.2 `
  <repo-root>\output_from_vm\team_network_v4_unified_runtime_role\ws63-liteos-app_v4_unified_all.fwpkg
```

## Local Verification

```sh
npm --prefix webui test
npm --prefix webui run build
git diff --check
```

Protocol smoke tests:

```sh
cc -Wall -Wextra -Werror -Iinclude -DSLE_TEAM_NETWORK_TEST \
  examples/team_network_demo.c src/sle_team_node.c src/sle_team_packet.c src/sle_team_web_api.c \
  -o /tmp/sle_team_network_test && /tmp/sle_team_network_test

cc -Wall -Wextra -Werror -Iinclude -DSLE_TEAM_PACKET_TEST \
  examples/team_node_common.c src/sle_team_packet.c \
  -o /tmp/sle_team_packet_test && /tmp/sle_team_packet_test
```

## License

Apache-2.0
