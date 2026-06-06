# Version v4.4.94

Date: 2026-06-06

## Scope

`v4.4.94` adds stable relay optimization on top of the dynamic relay budget from `v4.4.93`.

The new behavior answers the stable-topology question: after the relay count has already reached the target, the leader may exchange relay roles only when a non-relay member is clearly better than the worst active relay for long enough to prove it is not a transient RSSI spike.

## Design

- Firmware visible version bumped to `v4.4.94`.
- Added relay swap hysteresis:
  - candidate must be a non-relay eligible relay candidate.
  - victim is the worst active relay.
  - both RSSI values must be known.
  - candidate RSSI must be at least `8 dB` stronger than victim RSSI.
  - the same candidate/victim pair must satisfy that gap for `30 seconds`.
- Swap observation only runs when:
  - relay count equals relay target.
  - relay target is non-zero.
  - no pending members are waiting.
  - known member count equals current online count.
  - relay failover grace is not active.
- When the stable window completes, the leader sends `swap-promote` to the stronger candidate and `swap-demote` to the weaker active relay.

## Verification

- WebUI/firmware contract tests: pass.
- Python compile for WS63 build/flash/test helpers: pass.
- Python WS63 helper tests:
  - `test_ws63_auto_burn.py`: pass, 11 tests.
  - `test_ws63_four_board_relay_test.py`: pass, 20 tests.
- Remote Ubuntu firmware build: pass.
  - Host: `owen@192.168.6.5`
  - SDK: `/home/owen/workspace/bearpi-pico_h3863`
  - Build command: `python3 build.py -c ws63-liteos-app -j4`
  - Remote post-build guard: pass; ELF/package evidence contains `v4.4.94`, `relay swap observe`, `swap-promote`, `swap-demote`, and `v4.4.94 relay swap hysteresis`.
  - Local package: `output_from_vm/team_network_v4_unified_runtime_role/ws63-liteos-app_v4_unified_all.fwpkg`
  - Package size: `1,604,456` bytes.
  - SHA256: `3274B3AE49213589F1E8D6BFEDC238B04171B41A2462AD1A273466D4FF591543`
- `git diff --check`: pass, CRLF normalization warnings only.
- Hardware flash/burn: pass on COM16, COM13, COM17 and COM18.
  - Burn log directory: `logs\burn\v4.4.94_20260606_121042`
  - All four ports reported ymodem transfer completion and exit code 0.
- Live four-board direct-cap relay validation: pass.
  - Test log directory: `logs\live\v4.4.94_four_board_20260606_121503`
  - Topology: COM16 leader, COM13 initial relay/member, COM17 child1/member, COM18 child2/member.
  - Leader config evidence: `cfg direct 1` produced `relay_budget=1`.
  - Initial route evidence: three online members converged as `direct=1 relayed=2`.
  - Member reboot evidence: COM17 route 224 rebooted and rejoined; leader saw HELLO/rejoin and member table online.
  - Relay reboot evidence: COM13 route 241 loss triggered relay failover; COM18 route 86 was auto-promoted to relay.
  - Recovery policy observed: original COM13 relay returned as a normal member; COM18 retained the relay role.
