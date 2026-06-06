# Manifest: v4.4.98

## Updated Files

- `README.md`
- `docs/version_management.md`
- `docs/v4/README.md`
- `firmware/README.md`
- `versions/README.md`
- `versions/v4.4.98/VERSION.md`
- `versions/v4.4.98/MANIFEST.md`
- `webui/tests/ws63-api-contract.test.mjs`

## Cleanup Rationale

The previous README was accurate but read like an internal engineering note. This release makes the GitHub landing page easier to scan for external readers while preserving the repository's factual version and validation boundaries.

## Validation Commands

```sh
git diff --check
npm --prefix webui test
```
