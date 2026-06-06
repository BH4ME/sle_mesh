# v4.4.59 Flash And Four-Board Relay Test

## Flash Command With Visible Logs

Sequential flashing, safest when manual reset is needed:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\ws63_flash_multi.ps1 -Ports COM16,COM13,COM17,COM18 -ExpectedVersion v4.4.59
```

Parallel flashing, faster but only recommended when each board can be reset at
the right time:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\ws63_flash_multi.ps1 -Ports COM16,COM13,COM17,COM18 -ExpectedVersion v4.4.59 -Parallel
```

Each run writes logs under:

```text
logs/burn/v4.4.59_<timestamp>/<COM>.log
```

Progress lines to watch:

```text
Waiting for device reset...
Auto handshake timeout. Please press reset / BOOT+RESET now...
Establishing ymodem session...
Transferring ...
Done. Reseting device...
```

Do not claim a board is flashed until its log exits `0` and contains
`Done. Reseting device...`.

## Four-Board Roles

Observed board identities:

```text
COM16 leader: suffix 279A, route/self 154
COM13 relay candidate: suffix E7F1, route/self 241
COM17 member candidate: suffix E7E0, route/self 224
COM18 member candidate: suffix 5556, route/self 86
```

Runtime config:

```text
COM16: cfg leader now 1 17
COM13: cfg member now 279A 1 17
COM17: cfg member now 279A 1 17
COM18: cfg member now 279A 1 17
```

Pairing flow:

```text
COM16: pairing start
COM16: pairing approve 241 relay
COM16: pairing approve 224 norelay
COM16: pairing approve 86 norelay
COM16: pairing stop
```

## v4.4.58 Failure Evidence To Recheck

The failed `v4.4.58` run is saved under:

```text
logs/live/v4.4.58_four_board_join_20260604_194925/
logs/live/v4.4.58_post_fail_probe_20260604_200013/
```

Important observed lines:

```text
[sle uart client] will connect addr:52:**:**:**:e7:f1 count:0
[sle uart client] start scan skipped: seek active conn:0
[cli-rx] team=1 self=241 leader=154 role=0 state=2 joined=0
[sle-tx-fail] type=PACKET dst=154 ret=-4 reason=NOT_READY
```

For `v4.4.59`, the leader log must additionally show `connect request addr:` or
the timeout fallback before we treat the root cause as fixed.

## Live Test Result

Pending. Fill this section with:

- Build log path.
- Flash log paths for `COM16`, `COM13`, `COM17`, and `COM18`.
- Final `cfg status/state/members` evidence.
- Downstream member reboot result.
- Relay reboot/failover result.
- Original relay recovery behavior.
