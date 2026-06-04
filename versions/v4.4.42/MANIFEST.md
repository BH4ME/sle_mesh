# v4.4.42 Manifest

## Changed Files

- `include/sle_team_node.h`
- `src/sle_team_node.c`
- `examples/team_node_regression_test.c`
- `xc/ws63_team_network/src/ws63_team_network_app.c`
- `scripts/ws63_build_v4_ubuntu.sh`
- `automation/ws63/README.md`
- `webui/tests/ws63-api-contract.test.mjs`
- `README.md`
- `versions/README.md`
- `versions/v4.4.42/VERSION.md`
- `versions/v4.4.42/MANIFEST.md`

## Key Deltas

1. Added an explicit link-loss recovery API separate from manual leave.
2. Replaced the firmware disconnect fallback that called `sle_team_node_member_leave()` with `sle_team_node_member_link_lost()`.
3. Added regression coverage proving link loss preserves leader config and retries HELLO.
4. Updated WebUI contract tests to enforce the new split: manual leave is explicit, link loss auto-recovers.
5. Bumped visible firmware/build guard strings to `v4.4.42`.

## Verification

Passed locally:

```sh
bash -lc "cc -Wall -Werror -I/mnt/e/codex_documents/sle/include /mnt/e/codex_documents/sle/examples/team_node_regression_test.c /mnt/e/codex_documents/sle/src/sle_team_packet.c /mnt/e/codex_documents/sle/src/sle_team_node.c -o /tmp/team_node_regression_test && /tmp/team_node_regression_test"
python -m unittest automation.ws63.tests.test_ws63_link_cycle_test
bash scripts/simulate_v2.sh --suite=all --iterations=1
bash automation/ws63/scripts/ws63_test_system.sh --quick
npm --prefix webui test
npm --prefix webui run build
git diff --check
```

Results:

```text
[team-node-regression] pass
Ran 5 tests in 0.104s OK
[sim] summary: pass=1 fail=0 total=1
ws63_test_system.sh --quick: PASS
webui test: 53/53 pass
webui build: PASS
git diff --check: no whitespace errors, CRLF warnings only
```

Blocked live validation:

- Remote Ubuntu build host `owen@192.168.6.5` rejected non-interactive SSH: `Permission denied (publickey,password)`.
- Local Windows serial enumeration only showed `COM1`; `COM13` and `COM16` were not visible.

Live COM13/COM16 validation still must confirm when hardware/remote access is available:

1. Member reboot/signal loss: leader offline then online without manual join.
2. Manual leave: leader offline immediately, no auto-rejoin, manual rejoin succeeds.
3. Relay parent loss: child reselects/reconnects without clearing leader config.
