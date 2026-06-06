# Version v4.4.62

Date: 2026-06-05

## Scope

`v4.4.62` continues the four-board direct-cap relay recovery work from `v4.4.61`.

Target topology:

- `COM16`: leader, route/self id `154`, MAC suffix `279A`
- `COM13`: relay candidate, route/self id `241`, MAC suffix `E7F1`
- `COM17`: child member, route/self id `224`, MAC suffix `E7E0`
- `COM18`: child member, route/self id `86`, MAC suffix `5556`

Leader config:

```text
cfg leader now 1 17
cfg direct 1
```

## Root Cause

`v4.4.61` proved enrollment and child reboot recovery, but failed when the relay rebooted:

```text
enrollment PASS: leader=154 relay=241 child1=224 child2=86
child reboot PASS: leader saw offline and rejoin
FAIL: no child relay elected after relay loss
```

The leader cleared child shadow routes behind the lost relay and then pruned those children as heartbeat-timeout members during the same recovery window. Relay promotion also rolled back when CONFIG notify could not be delivered immediately.

## What Changed

1. Firmware version bumped to `v4.4.62`.
2. Relay-offline handling records downstream children before route cleanup.
3. A 30 second failover grace window preserves watched child routes and defers timeout for those children.
4. Leader seek can temporarily connect watched child members even when they are not normal bucket-1 direct candidates.
5. Relay promotion no longer rolls back on temporary CONFIG notify failure; it logs `relay config notify pending` and retries every 2 seconds.
6. Remote build guard checks `v4.4.62`, relay failover logs, and relay CONFIG retry logs.

## Required Live Verification

This version is accepted only when serial logs prove:

1. All four boards report `fw:"v4.4.62"` from `cfg status`.
2. `COM16` accepts `cfg leader now 1 17` and `cfg direct 1`.
3. `COM13`, `COM17`, and `COM18` accept `cfg member now 279A 1 17`.
4. Pairing enrolls `241` as relay and `224/86` as normal members with `ret=0`.
5. Route metrics settle to `active=3 direct=1 relayed=2 stale=0 unreachable=0 plan=0 converged=1`.
6. Rebooting child1 produces leader offline/lost then online/rejoin evidence.
7. Rebooting relay produces leader relay-offline evidence, child relay election evidence, and all three members online after original relay recovery.
8. Final log states whether original relay regained relay role or returned as member.

## Command Log Slots

Fill these after live verification:

```text
build log:
burn log:
four-board test log:
final policy:
```
