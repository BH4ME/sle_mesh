# Version v4.4.61

Date: 2026-06-04

## Scope

`v4.4.61` is a root-cause fix for the four-board relay enrollment failure found after `v4.4.60`.

Test topology:

- `COM16`: leader, route/self id `154`, MAC suffix `279A`
- `COM13`: relay member, route/self id `241`, MAC suffix `E7F1`
- `COM17`: child member, route/self id `224`, MAC suffix `E7E0`
- `COM18`: child member, route/self id `86`, MAC suffix `5556`

Leader config:

```text
cfg leader now 1 17
cfg direct 1
```

Expected topology is one direct relay plus two child members through that relay.

## Root Cause

Live logs proved the leader could see child traffic through the relay, but approval failed:

```text
[sle-tx-fail] type=PACKET dst=224 ret=-4 reason=NO_ROUTE
[cli-rx] pairing approve member=224 relay=0 ret=-4
[team] route metrics active=3 direct=1 relayed=2 stale=0 unreachable=3 plan=0 converged=0
```

The physical SLE connection for the relay used `conn_id:0`:

```text
[sle uart client] bind member:241 conn_id:0
```

`conn_id=0` is valid on this WS63 SLE stack. The bug was in `team_route_conn_is_active()`, which rejected `conn_id == 0U` before asking the owner connection table. That made a real relay route look unreachable and prevented the leader from sending `CONFIG` to child nodes through the relay.

## What Changed

1. Firmware version bumped to `v4.4.61`.
2. `team_route_conn_is_active()` no longer treats `conn_id=0` as invalid.
3. Route liveness now depends on `sle_uart_client_has_conn(conn_id)` or `sle_uart_server_has_conn(conn_id)`.
4. WebUI contract tests now lock the `conn_id=0` behavior so the early reject is not reintroduced.
5. Build and flash defaults now expect `v4.4.61`.

## Expected Behavior

After enrollment with `COM16` leader and `cfg direct 1`:

```text
[cli-rx] pairing approve member=241 relay=1 ret=0
[cli-rx] pairing approve member=224 relay=0 ret=0
[cli-rx] pairing approve member=86 relay=0 ret=0
[team] route metrics active=3 direct=1 relayed=2 stale=0 unreachable=0 plan=0 converged=1
```

## Required Live Verification

This version is not accepted until serial logs prove:

1. All four boards report `fw:"v4.4.61"` from `cfg status`.
2. `COM16` accepts `cfg leader now 1 17` and `cfg direct 1`.
3. `COM13`, `COM17`, and `COM18` accept `cfg member now 279A 1 17`.
4. Pairing enrolls `241` as relay and `224/86` as normal members with `ret=0`.
5. Route metrics settle to `active=3 direct=1 relayed=2 stale=0 unreachable=0 plan=0 converged=1`.
6. Rebooting a child produces leader offline/lost then online/rejoin evidence.
7. Rebooting relay produces leader relay-offline evidence and downstream recovery or relay re-election evidence.
