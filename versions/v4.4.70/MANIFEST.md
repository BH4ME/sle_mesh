# v4.4.70 Manifest

Date: 2026-06-05

## Files Added

```text
versions/v4.4.70/VERSION.md
versions/v4.4.70/MANIFEST.md
versions/v4.4.70/COM16_BLOCKER.md
```

## Files Updated

```text
README.md
versions/README.md
```

## Firmware

No new firmware image was produced for this record.

```text
current firmware baseline: v4.4.66
firmware package: E:\codex_documents\sle\output_from_vm\team_network_v4_unified_runtime_role\ws63-liteos-app_v4_unified_all.fwpkg
```

## Validation Evidence

COM16 leader probe:

```text
command: automation/ws63/tools/ws63_four_board_relay_test.py with COM16 as leader
log: E:\codex_documents\sle\logs\live\v4.4.69_four_board_com16_probe_20260605_100307
result: FAIL, COM16 returned 0 bytes to cfg status
```

Three-board fallback relay test:

```text
command: automation/ws63/tools/ws63_relay_cycle_test.py with COM13 leader, COM17 relay, COM18 child
log: E:\codex_documents\sle\logs\live\v4.4.69_three_board_fallback_20260605_100452
result: PASS
```

Local Python unit tests:

```text
command: .\.tooling\py311\python.exe -m pytest automation\ws63\tests\test_ws63_four_board_relay_test.py automation\ws63\tests\test_ws63_auto_burn.py -q
result: NOT RUN, bundled .tooling Python has no pytest module installed
```

## Current Open Item

The user-requested four-board topology remains open until `COM16` either:

```text
returns [cfg-json] with fw=v4.4.66
enters ROM boot handshake during BOOT/RST
is replaced by a responsive board for the leader role
```
