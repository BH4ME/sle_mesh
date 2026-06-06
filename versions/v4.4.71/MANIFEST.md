# v4.4.71 Manifest

Date: 2026-06-05

## Files Added

```text
versions/v4.4.71/VERSION.md
versions/v4.4.71/MANIFEST.md
```

## Files Updated

```text
README.md
versions/README.md
```

## Firmware

No firmware rebuild was performed.

```text
current firmware baseline: v4.4.66
```

## Validation Evidence

COM16 leader probe:

```text
command: automation/ws63/tools/ws63_four_board_relay_test.py with COM16 as leader
log: <repo-root>\logs\live\v4.4.70_four_board_com16_probe_20260605_101328
result: FAIL, COM16 returned 0 bytes to cfg status
```

Script syntax:

```text
command: .\.tooling\py311\python.exe -m py_compile automation\ws63\tools\ws63_four_board_relay_test.py automation\ws63\tools\ws63_relay_cycle_test.py automation\ws63\tools\ws63_auto_burn.py
result: PASS
```

Whitespace check:

```text
command: git diff --check -- README.md versions/README.md versions/v4.4.70/VERSION.md versions/v4.4.70/MANIFEST.md versions/v4.4.70/COM16_BLOCKER.md
result: PASS, only CRLF conversion warnings were printed
```

## Open Requirement

The user-requested full four-board test remains unverified until the designated
leader port `COM16` can be controlled by the test script.
