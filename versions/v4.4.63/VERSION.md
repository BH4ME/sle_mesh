# Version v4.4.63

Date: 2026-06-05

## Scope

`v4.4.63` continues the four-board direct-cap relay verification from `v4.4.62`.

Target topology:

- `COM16`: leader, route/self id `154`, MAC suffix `279A`
- `COM13`: original relay candidate, route/self id `241`, MAC suffix `E7F1`
- `COM17`: child member, route/self id `224`, MAC suffix `E7E0`
- `COM18`: child member, route/self id `86`, MAC suffix `5556`

Leader config:

```text
cfg leader now 1 17
cfg direct 1
```

## Root Cause

Live `v4.4.62` testing proved enrollment and child1 reboot/rejoin, but child1 reboot also exposed relay-role churn:

```text
child-reboot PASS: leader saw child1 offline and child1 rejoined
leader: relay offline event member=241 trigger immediate rebalance
leader: relay set member=86 allow=1 notify=1 reason=auto-promote ret=0
```

The original relay was still sending heartbeats, so the failure was not proven to be a real relay loss. Code review showed that `sle_team_handle_hello()` refreshed `online`, `last_seen_s`, and `last_seq`, then rolled the entire member record back when leader-to-member CONFIG/ACK delivery failed. In a four-board relay topology this can make a live member look stale and trigger heartbeat timeout/rebalance.

## What Changed

1. Firmware version bumped to `v4.4.63`.
2. Leader HELLO handling now preserves member liveness (`online`, `last_seen_s`, `last_seq`, identity fields) when CONFIG or ACK delivery fails for an already-known member.
3. The failure log now records `liveness preserved` so live serial logs can prove this path is active.
4. Remote build guard checks the `v4.4.63` version string and the liveness-preservation log strings.

## Required Live Verification

This version is accepted only when serial logs prove:

1. All four boards report `fw:"v4.4.63"` from `cfg status`.
2. `COM16` is leader with `runtimeDirectCap=1`.
3. `COM13`, `COM17`, and `COM18` are members of leader suffix `279A`.
4. Pairing/enrollment yields `241` as relay and `224/86` as normal members.
5. Route metrics settle to `active=3 direct=1 relayed=2 stale=0 unreachable=0 plan=0 converged=1`.
6. Rebooting child1 produces leader timeout/offline evidence for `224`, then `224 online=1` and child1 `joined=1`, without unintended relay churn when the relay remains healthy.
7. Rebooting the relay produces leader relay-offline evidence, child relay election evidence, and all three members online after original relay recovery.
8. Final log states whether original relay regained relay role or returned as a normal member.

## Command Log Slots

Current evidence:

```text
build log: E:\codex_documents\sle\logs\build\v4.4.63_20260605_011817\remote_build.log
firmware package: E:\codex_documents\sle\output_from_vm\team_network_v4_unified_runtime_role\ws63-liteos-app_v4_unified_all.fwpkg
burn log, parallel attempt: E:\codex_documents\sle\logs\burn\v4.4.63_20260605_012433
burn log, COM17 foreground failed: E:\codex_documents\sle\logs\burn\v4.4.63_COM17_foreground_20260605_013025\COM17.log
burn log, COM17 visible failed: E:\codex_documents\sle\logs\burn\v4.4.63_COM17_visible_20260605_043820\COM17.log
four-board test log: pending
child reboot log: pending
relay reboot log: pending
final policy: pending
```

Known burn status from `cfg status` on 2026-06-05:

```text
COM16: fw="v4.4.63", suffix=279A, route=154
COM13: fw="v4.4.63", suffix=E7F1, route=241
COM17: fw="v4.4.62", suffix=E7E0, route=224
COM18: fw="v4.4.62", suffix=5556, route=86
```

`COM17` and `COM18` still need to be flashed to `v4.4.63` before live four-board acceptance can start.

## Flash Procedure To Reuse

Use local Windows for flashing and remote Ubuntu only for building.

Single-port visible flash:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\ws63_flash_multi.ps1 `
  -Ports COM17 `
  -ExpectedVersion v4.4.63 `
  -WaitTimeout 15 `
  -ManualRetryTimeout 300
```

Parallel flash:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\ws63_flash_multi.ps1 `
  -Ports COM16,COM13,COM17,COM18 `
  -ExpectedVersion v4.4.63 `
  -WaitTimeout 15 `
  -ManualRetryTimeout 300 `
  -Parallel
```

The script writes one run directory under `logs/burn/`, plus:

```text
run_summary.txt
<PORT>.log
<PORT>.command.txt
```

Manual boot timing:

```text
When a port log prints:
  Please press reset / BOOT+RESET now...

Do:
  1. Hold BOOT.
  2. Tap RESET/RST.
  3. Release BOOT.

Success evidence:
  Establishing ymodem session...
  Done. Reseting device...
```

Do not count a board as flashed unless `cfg status` later reports the expected firmware version.

## Four-Board Test Plan

After all four boards report `fw:"v4.4.63"`:

```text
COM16: cfg leader now 1 17
COM16: cfg direct 1
COM13: cfg member now 279A 1 17
COM17: cfg member now 279A 1 17
COM18: cfg member now 279A 1 17
```

Expected topology:

```text
Leader: COM16 route=154 suffix=279A direct_cap=1
Original relay candidate: COM13 route=241 suffix=E7F1
Child member 1: COM17 route=224 suffix=E7E0
Child member 2: COM18 route=86 suffix=5556
```

Acceptance evidence to capture in live logs:

```text
1. Pairing approves 241 as relay and 224/86 as normal members.
2. Leader members table: 241/224/86 online=1.
3. Route metrics: active=3 direct=1 relayed=2 stale=0 unreachable=0 plan=0 converged=1.
4. Reboot COM17: leader sees 224 offline, then 224 online=1/rejoined.
5. Reboot COM13 relay: leader sees relay offline; 224/86 self-heal through one elected relay.
6. Original relay recovery policy is recorded: original relay regains relay role, or returns as normal member.
```
