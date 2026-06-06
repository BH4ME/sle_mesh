# Version v4.4.39

Date: 2026-06-03

## Scope

`v4.4.39` records the local C demo verification pass and fixes issues found while running the examples with strict warnings, sanitizers, and GCC static analysis.

This version does not change the board firmware business logic, WebUI runtime behavior, display behavior, or the board-visible firmware version.

## What Changed

1. C demo packet parsing now copies decoded app bodies into aligned local structs before reading fields.
2. Relay demo seed helpers now assert that deterministic seed members exist before writing RSSI overrides.
3. The repository record version was advanced to `v4.4.39` so the demo-check fixes have a rollback point.

## Firmware Impact

None. The board-visible firmware version remains `v4.4.37`.

## Verification

- WSL GCC strict C demo pass: `-std=c99 -Wall -Wextra -Werror`.
- WSL ASAN/UBSAN pass: `-fsanitize=address,undefined` with halt-on-error.
- WSL GCC static analyzer pass: `-fanalyzer`.
- `git diff --check`: pass with only Windows LF-to-CRLF conversion warnings.
- `npm --prefix webui test`: 53/53 pass, including repository-version synchronization.
