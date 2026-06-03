# Version v4.4.38

Date: 2026-06-03

## Scope

`v4.4.38` is a repository hygiene and release-branch cleanup record.
It does not change firmware networking logic, display logic, WebUI runtime behavior, or the board-visible firmware version.

## What Changed

1. Remote GitHub branches with the old tool-specific prefix were copied to neutral `line/*` or `release/*` refs, then the old remote refs were deleted.
2. The current working branch was renamed to `release/v4.4.38`.
3. Tracked documentation paths were normalized from machine-specific absolute paths to neutral placeholders such as `<repo-root>`, `<sdk-root>`, and `<workspace-root>`.
4. Script example/default users were changed to the neutral `builder` placeholder.
5. The project SOP now uses `line/` as the default work-branch prefix.
6. Build and flash scripts keep runnable repo-local output defaults instead of placeholder runtime paths.

## Firmware Impact

None. The board-visible firmware version remains `v4.4.37`.

## Verification

- Remote branch check: no old tool-specific remote refs remain.
- Tracked content search for the old marker returns no matches.
- `git diff --check`: pass with only Windows line-ending conversion warnings.
