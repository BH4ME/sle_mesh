# Serial Bulk Config

Date: 2026-05-31

## Purpose

v4.4 adds a single configuration path shared by:

- firmware runtime/business logic
- firmware built-in WebUI and HTTP API
- external domain WebUI
- serial CLI and WebSerial

This is meant for 30+ node deployment where connecting to every board's WiFi is too slow.

## Firmware Serial Commands

Serial settings:

```text
115200 8N1
```

Commands:

```text
cfg status
cfg leader [team channel]
cfg leader now <team> <channel>
cfg member <leader_suffix_hex> <team> <channel>
cfg member now <leader_suffix_hex> <team> <channel>
cfg apply
cfg clear
cfg reboot
```

Behavior:

- `cfg status` prints human-readable status and one structured `[cfg-json] {...}` line.
- `cfg leader ...` and `cfg member ...` save NV config.
- `now` variants save NV config and queue immediate runtime role application.
- `cfg apply` applies the saved NV config.
- `cfg clear` clears saved config; reboot to return to unconfigured state.
- `cfg reboot` schedules a board reboot.

## HTTP API

The same config path is exposed over the board HTTP server:

```text
GET /api/config/status
GET /api/config/leader?team=1&channel=17&now=1
GET /api/config/member?leader=C7E9&team=1&channel=17&now=1
GET /api/config/apply
GET /api/config/clear
GET /api/config/reboot
```

HTTP action responses include:

```json
{
  "ok": true,
  "action": "leader-now",
  "ret": 0,
  "config": {
    "ok": true,
    "fw": "v4.4",
    "selfSuffix": "9A2F",
    "nvValid": true,
    "nvRole": "leader"
  }
}
```

## External WebUI Flow

Use the domain WebUI settings page:

1. Open `Settings`.
2. Select serial mode and click the serial connect button.
3. Use `One-click node config`.
4. Choose `Leader` or `Member`.
5. Fill `Team ID`, `Channel`, and for member mode the leader suffix.
6. Keep `Apply immediately` checked for fast deployment.
7. Click `Write config`.
8. Check the `Serial log` panel for `[cfg-json]` and `ret=0`.

The WebUI serial implementation sends the same CLI commands:

```text
cfg status
cfg leader now <team> <channel>
cfg member now <leader_suffix> <team> <channel>
cfg apply
cfg clear
cfg reboot
```

Important: the WebUI treats missing `ret=` from a write/apply command as failure, so stale `roleRequestLastRet=0` is not misreported as success.

## PowerShell Helper

Script:

```text
scripts/ws63_serial_cfg.ps1
```

Leader example:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/ws63_serial_cfg.ps1 -Port COM7 -Mode leader -Team 7 -Channel 33
```

Member example:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/ws63_serial_cfg.ps1 -Port COM8 -Mode member -LeaderSuffix 9A2F -Team 7 -Channel 33
```

Read back:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/ws63_serial_cfg.ps1 -Port COM8 -Mode status
```

Clear:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/ws63_serial_cfg.ps1 -Port COM8 -Mode clear
```

## Expected Serial Evidence

Leader:

```text
[cfg] leader-now queued ret=0 team=7 channel=33 self_suffix=9A2F
[cfg-json] {"ok":true,"fw":"v4.4",...,"nvRole":"leader","nvTeam":7,"nvChannel":33,...}
```

Member:

```text
[cfg] member-now queued ret=0 leader_suffix=9A2F leader=47 team=7 channel=33
[cfg-json] {"ok":true,"fw":"v4.4",...,"nvRole":"member","nvLeaderSuffix":"9A2F",...}
```

If `cfg apply` has no saved config:

```text
[cfg] apply failed ret=-2 reason=nv_empty
```

The exact negative value is the firmware format error code; the important part is that a `ret=` is present so tools do not treat it as success.
