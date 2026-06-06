# v4.4.95 - Pairing Allowlist Preserve

## Scope

- Preserve members that were already online when a leader pairing window is opened.
- Keep the v4.4.94 dynamic relay budget and relay swap hysteresis behavior unchanged.
- Support the natural four-board validation path where `cfg direct` is not forced and firmware decides whether relay roles are needed.

## Root Cause

The v4.4.94 natural/default-direct run used COM16 as leader with COM13, COM17, and COM18 as members. COM13 joined before `pairing start`. When the leader later ran `pairing start`, the member filter was enabled and pending state was cleared. `pairing stop` approved only pending members, so COM13 was not sealed into the allowlist. After the member reboot step, COM13 heartbeats were rejected by the leader allowlist.

## Fix

`sle_team_node_pairing_stop()` now snapshots existing leader member records and adds those member IDs to the allowlist before approving pending members. This preserves already-online devices and still lets pending members be approved as normal non-relay members by default.

## Verification

- Local source/tests:
  - `python -m py_compile` for the changed WS63 automation helpers passed.
  - `test_ws63_four_board_relay_test.py`: 25 tests passed.
  - `test_ws63_auto_burn.py`: 11 tests passed.
  - `npm --prefix webui test`: 60 tests passed.
- Remote Ubuntu firmware build:
  - Host: `owen@192.168.6.5`
  - SDK: `/home/owen/workspace/bearpi-pico_h3863`
  - Log: `logs/build/v4.4.95_remote_20260606_130024/remote_build.log`
  - Result: `Build target:ws63_liteos_app success`; post-build guard passed.
- Firmware package:
  - Path: `output_from_vm/team_network_v4_unified_runtime_role/ws63-liteos-app_v4_unified_all.fwpkg`
  - Size: `1,604,520` bytes
  - SHA256: `2EF3066BDCD21D7F8125BDD3B714302C980D5ADF8364F9590E2D088C2D6D1F77`
  - Guard strings found: `v4.4.95`, `v4.4.95 pairing allowlist preserve`, `runtimeRelayBudget`, `relay swap observe`, `swap-promote`, `swap-demote`.
- Four-board flash:
  - Ports: COM16, COM13, COM17, COM18
  - Log: `logs/burn/v4.4.95_20260606_130733`
  - Result: all four ports reported `Done. Reseting device...` and exit code 0.
- Natural four-board run without `cfg direct`:
  - Log: `logs/live/v4.4.95_natural_default_direct_20260606_131107`
  - Result: PASS, total `68,142 ms`.

## Expected Natural Four-Board Result

With the default leader direct capacity, the leader should report `runtimeDirectCap=8` and `runtimeRelayBudget=3`. With only three members, firmware is expected to keep all three direct (`active=3 direct=3 relayed=0`) and elect no relay. If no relay is elected, the relay reboot/failover branch is not applicable for this natural-capacity run.

## Observed Natural Four-Board Result

- Role/id map:
  - COM16 leader: route `154`, suffix `279A`
  - COM13 member: route `241`, suffix `E7F1`
  - COM17 member: route `224`, suffix `E7E0`
  - COM18 member: route `86`, suffix `5556`
- Leader capacity evidence: `runtimeDirectCap=8`, `runtimeRelayBudget=3`.
- Natural enrollment topology: `relays=[]`, so no relay was elected.
- Leader route evidence after member reboot: `active=3 direct=3 relayed=0 stale=0 unreachable=0 plan=0 converged=1`.
- Member reboot evidence: COM17/route `224` rebooted, restored member config, sent HELLO to leader, and rejoined online.
- No `heartbeat rejected by allowlist` or `position rejected by allowlist` event was found in the v4.4.95 natural run logs.
- Since no relay was elected under the default direct capacity, relay reboot/failover/recovery behavior was not exercised in this natural run.
