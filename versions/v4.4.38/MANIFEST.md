# v4.4.38 Manifest

## Changed Files

- `README.md`
- `docs/README.md`
- `docs/v0/README.md`
- `docs/v1/README.md`
- `docs/v2/README.md`
- `docs/v4/README.md`
- `docs/v4/findings.md`
- `meta/DOC_WORKFLOW.md`
- `meta/PROJECT_OPERATION_SOP.md`
- `meta/review_feedback.md`
- `scripts/ws63_build_team_ubuntu.sh`
- `scripts/ws63_build_team_vm.sh`
- `scripts/ws63_build_v4_ubuntu.sh`
- `scripts/ws63_flash_team.sh`
- `versions/README.md`
- `versions/v4.4.38/VERSION.md`
- `versions/v4.4.38/MANIFEST.md`
- Historical version notes and VM setup notes containing local path/user placeholders.

## Key Deltas

1. Public remote branch names are neutralized and the latest commit is preserved on `release/v4.4.38`.
2. Documentation no longer exposes tool-specific branch prefixes, local profile names, or machine-specific absolute paths.
3. Build/flash scripts use neutral `builder` examples while preserving environment-variable overrides.
4. Runtime defaults for output directories remain usable under the repository root.

## Verification

Completed before commit/push:

- Remote branch search for the old marker: no matches.
- Tracked content search for the old marker: no matches.
- `git diff --check`: pass with only Windows LF-to-CRLF warnings.
