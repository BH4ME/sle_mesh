# v4.4.58 Flash And Four-Board Relay Test

## Visible Flash Command

Sequential flashing, safest when manual reset is needed:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\ws63_flash_multi.ps1 -Ports COM16,COM13,COM17,COM18
```

Parallel flashing, faster but requires careful manual button timing:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\ws63_flash_multi.ps1 -Ports COM16,COM13,COM17,COM18 -Parallel
```

Each run writes logs under:

```text
logs/burn/v4.4.58_<timestamp>/<COM>.log
```

Progress lines to watch:

```text
Waiting for device reset...
Auto handshake timeout. Please press reset / BOOT+RESET now...
Establishing ymodem session...
Transferring ...
Done. Reseting device...
```

If a board does not enter burn mode automatically, press `BOOT+RESET` or `RESET/RST` while its log is waiting for reset. Do not claim a board is flashed until that port log exits `0` and contains `Done. Reseting device...`.

## Four-Board Roles

Current observed board identities:

```text
COM16 leader candidate: suffix 279A, route/self 154
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
COM16: pairing approve 224
COM16: pairing approve 86
```

Expected steady state on leader:

```text
member=241 online=1 relay=1 tier=1
member=224 online=1 relay=0 parent/next hop through 241
member=86 online=1 relay=0 parent/next hop through 241
```

Each member must also report `joined=1`; leader-only membership is not enough evidence.

## Test Matrix

1. Downstream member reboot:
   - Reboot `COM17` or `COM18`.
   - Leader must show offline/lost for that member.
   - After reboot, the member must restore flash config and rejoin automatically.

2. Relay reboot:
   - Reboot `COM13`.
   - Leader must show relay offline/lost.
   - `COM17` and `COM18` must not remain permanently stuck under the dead parent.
   - Record whether one child is promoted/self-heals as relay or whether both reconnect directly/pending based on leader policy.

3. Original relay recovery:
   - Let `COM13` boot again.
   - Record whether `COM13` regains relay status or returns as a normal member.
   - Do not assume; verify with `members`, serial logs, and each member `cfg status`.

## Why v4.4.58 Exists

Before this version, `relay_discovery_only` blocked `CONFIG` and `ACK`.
That created a split-brain join state:

```text
leader: pending child approved / member record exists
relay: received downlink CONFIG/ACK but node packet ret=-4
child: joined=0 / NOT_READY
```

`v4.4.58` allows `CONFIG` and `ACK` through discovery-only relay mode while keeping business packets blocked.

## Actual Live Result

`v4.4.58` did flash and configure correctly on all four boards, but the live
four-board join did not reach approval. The root cause moved below the pairing
protocol:

```text
leader: [sle uart client] will connect addr:52:**:**:**:e7:f1 count:0
leader: [sle uart client] start scan skipped: seek active conn:0
relay:  [cli-rx] team=1 self=241 leader=154 role=0 state=2 joined=0
relay:  [sle-tx-fail] type=PACKET dst=154 ret=-4 reason=NOT_READY
```

Evidence logs:

```text
logs/live/v4.4.58_four_board_join_20260604_194925/
logs/live/v4.4.58_post_fail_probe_20260604_200013/
```

This was fixed in `v4.4.59` by adding SLE seek-stop connect recovery.
