# Version v4.4.60

Date: 2026-06-04

## Scope

`v4.4.60` fixes the relay recovery failure found in the live four-board test
after `v4.4.59` could connect and enroll boards.

The failing setup was:

- `COM16`: leader, route/self id `154`, MAC suffix `279A`
- `COM13`: relay member, route/self id `241`, MAC suffix `E7F1`
- `COM17`: child member, route/self id `224`, MAC suffix `E7E0`
- `COM18`: child member, route/self id `86`, MAC suffix `5556`

With leader `cfg direct 1`, the intended topology is one direct relay plus two
downstream children. The leader log instead showed:

```text
[team] relay set member=241 allow=0 notify=1 reason=auto-demote ret=0
[team] relay rebalance online=3 relay=0 target=0 changed=1
```

That demoted the only relay, so child members could end at `joined=0` and could
not recover cleanly after member or relay reboot.

## Root Cause

1. `team_leader_relay_target_for_online()` used the hardware direct connection
   maximum instead of the runtime direct-cap requested by `cfg direct 1`.
2. The leader could bind a relay-forwarded child packet to the physical relay
   connection as if the child itself were directly connected.
3. Member-to-leader sends could still broadcast upstream instead of preferring
   the selected upstream parent connection.

## What Changed

1. Added leader runtime direct-cap support:
   - `cfg direct`
   - `cfg direct <1-8>`
   - `/api/config/status` field `runtimeDirectCap`
2. Relay target calculation now uses `team_leader_direct_capacity()`.
3. Leader route metrics now include route-plan mismatch:
   - `plan=%u`
   - `converged=1` requires no stale, unreachable, or plan-mismatched route.
4. Leader desired-parent planning chooses whether a member should stay direct or
   use an online relay parent.
5. Leader connection binding keeps physical first-hop relay identity separate
   from logical child route identity.
6. Member upstream sends prefer `upstream_parent_id` when sending to leader.

## Expected Behavior

With `COM16` configured as leader and `cfg direct 1`:

- `COM13` should remain relay-enabled when three members are online.
- `COM17` and `COM18` should route through the relay when direct capacity is
  exhausted.
- Leader route metrics should settle to:

```text
[team] relay rebalance online=3 relay=1 target=1
[team] route metrics active=3 direct=1 relayed=2 stale=0 unreachable=0 plan=0 converged=1
```

## Required Live Verification

This version is not accepted until serial logs prove:

1. All four boards report `fw=v4.4.60`.
2. `COM16` accepts `cfg leader now 1 17` and `cfg direct 1`.
3. `COM13`, `COM17`, and `COM18` accept `cfg member now 279A 1 17`.
4. Pairing enrolls `241` as relay and `224/86` as normal members.
5. `pairing stop` does not demote relay `241`.
6. Rebooting a child produces leader offline/lost then online/rejoin.
7. Rebooting relay produces leader relay-offline evidence and downstream
   recovery or relay re-election evidence.
