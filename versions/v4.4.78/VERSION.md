# Version v4.4.78

Date: 2026-06-05

## Scope

`v4.4.78` is the follow-up code fix after reviewing the `v4.4.74` feedback
chain and the `v4.4.76` four-board logs. No flashing is part of this version
creation step.

Evidence reviewed:

```text
logs/live/v4.4.74_four_board_com16_leader_20260605_135942/
logs/live/v4.4.75_four_board_com16_leader_20260605_144113/
logs/live/v4.4.76_four_board_com16_leader_20260605_155722/
```

## Root Cause

`v4.4.77` prevented leader-origin `CONFIG` and `ACK` from logically clearing a
relay reselect target, but the physical SLE link was still not forced to move.

In the failing logs, children repeatedly printed:

```text
route update requests parent reselect
upstream parent=154 state=2 reason=packet
```

That means the child accepted the leader's reselect policy, but its old
leader-side ACL connection remained alive. The member SLE server had also
terminated announce after the leader connected, so the relay could not reliably
discover and adopt the child. The leader kept sending `direct cap migrate`, but
route metrics stayed `direct=3 relayed=0`.

## Fix

- Firmware version is bumped to `v4.4.78`.
- Leader direct-cap enforcement now sends the reselect hint and then disconnects
  the old direct child ACL instead of waiting forever for a route that cannot
  physically form while the old ACL is still held.
- Member reselect handling now drops the old leader/upstream connection and
  restarts SLE advertising while a non-leader parent target is pending.
- Seek-filter rejection paths now log a short reason so the next `filter:0`
  case can be diagnosed from logs instead of guessing.
- Build guards now require the new parent-reselect disconnect strings in the
  ELF before any flash is accepted.

## Required Verification

Run before any flashing:

```powershell
git diff --check

.\.tooling\py311\python.exe -m py_compile `
  automation\ws63\tools\ws63_four_board_relay_test.py `
  automation\ws63\tools\ws63_remote_build_v4.py `
  automation\ws63\tools\ws63_auto_burn.py

.\.tooling\py311\python.exe -m unittest `
  automation.ws63.tests.test_ws63_four_board_relay_test `
  automation.ws63.tests.test_ws63_auto_burn
```

Then build on `192.168.6.5` with:

```powershell
$env:UBUNTU_HOST='192.168.6.5'
$env:UBUNTU_USER='owen'
$env:UBUNTU_PASS='<local only>'
$env:UBUNTU_SDK='/home/owen/workspace/bearpi-pico_h3863'
.\.tooling\py311\python.exe .\automation\ws63\tools\ws63_remote_build_v4.py
```

Flash only after these checks pass and only when flashing is explicitly
requested.
