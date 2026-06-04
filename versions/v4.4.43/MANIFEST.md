# v4.4.43 Manifest

## Changed Files

- `automation/ws63/tests/test_ws63_link_cycle_test.py`
- `scripts/ws63_build_v4_ubuntu.sh`
- `xc/ws63_team_network/src/ws63_team_network_app.c`
- `webui/tests/ws63-api-contract.test.mjs`
- `README.md`
- `versions/README.md`
- `versions/v4.4.43/VERSION.md`
- `versions/v4.4.43/MANIFEST.md`

## Key Deltas

1. Added an automation unit test proving the reboot cycle does not manually rejoin before `leave`.
2. Kept the `v4.4.42` firmware semantics: manual leave clears active leader; link loss preserves leader and auto-recovers.
3. Updated visible firmware/build guard strings to `v4.4.43`.

## Verification

Passed locally:

```sh
python -m unittest automation.ws63.tests.test_ws63_link_cycle_test
bash -lc "cc -Wall -Werror -I/mnt/e/codex_documents/sle/include /mnt/e/codex_documents/sle/examples/team_node_regression_test.c /mnt/e/codex_documents/sle/src/sle_team_packet.c /mnt/e/codex_documents/sle/src/sle_team_node.c -o /tmp/team_node_regression_test && /tmp/team_node_regression_test"
bash scripts/simulate_v2.sh --suite=all --iterations=1
bash automation/ws63/scripts/ws63_test_system.sh --quick
npm --prefix webui test
npm --prefix webui run build
git diff --check
```

Results:

```text
link-cycle unit tests: 6/6 pass
[team-node-regression] pass
[sim] summary: pass=1 fail=0 total=1
ws63_test_system.sh --quick: PASS
webui test: 53/53 pass
webui build: PASS
git diff --check: no whitespace errors, CRLF warnings only
```

Blocked live validation:

- Remote Ubuntu build host `owen@192.168.6.5` rejected non-interactive SSH: `Permission denied (publickey,password)`.
- Local Windows serial enumeration only showed `COM1`; `COM13` and `COM16` were not visible.

Live COM13/COM16 validation is still required when serial ports and the Ubuntu build host are available.
