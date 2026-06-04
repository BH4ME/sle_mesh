# v4.4.50 Manifest

## Changed Files

- `automation/ws63/tools/ws63_relay_cycle_test.py`
- `automation/ws63/tests/test_ws63_relay_cycle_test.py`
- `automation/ws63/tests/test_ws63_system_script.py`
- `automation/ws63/scripts/ws63_test_system.sh`
- `README.md`
- `versions/README.md`
- `webui/tests/ws63-api-contract.test.mjs`
- `versions/v4.4.50/VERSION.md`
- `versions/v4.4.50/MANIFEST.md`

## Key Deltas

1. Added a live three-board relay failover tool for leader + relay member + child member.
2. The tool validates relay reboot/signal loss, not manual leave.
3. The tool validates child failover without issuing a child manual rejoin after relay loss.
4. System automation now has an optional `--with-relay-cycle` stage.
5. Repository version advanced to `v4.4.50`; firmware-visible version remains `v4.4.48`.

## Verification

Completed on 2026-06-04:

```sh
python -m unittest automation.ws63.tests.test_ws63_relay_cycle_test automation.ws63.tests.test_ws63_system_script automation.ws63.tests.test_ws63_link_cycle_test
bash automation/ws63/scripts/ws63_test_system.sh --quick
npm --prefix webui test
git diff --check
python automation/ws63/tools/ws63_link_cycle_test.py --leader-port COM16 --member-port COM13 --bootstrap-roles --team-id 1 --channel 17 --member-id 241 --state-timeout-s 45 --reboot-offline-timeout-s 30 --member-boot-timeout-s 60 --no-auto-rejoin-s 8 --log-dir logs/auto_test/v4.4.50_live_link_cycle_reconfirm
```

Observed results:

- Relay/link-cycle focused unit tests: `Ran 11 tests ... OK`
- Broader Python unit set: `Ran 29 tests ... OK`
- `npm --prefix webui test`: `54` tests passed.
- `bash automation/ws63/scripts/ws63_test_system.sh --quick`: final `PASS`, including `unit_ws63_relay_cycle` and `sim_python_1v30_relay_failover`.
- `git diff --check`: no whitespace errors; only CRLF normalization warnings.
- COM16 leader + COM13 member live reconfirm: `[link-cycle] PASS: reboot restore + manual leave/rejoin`, logs in `logs/auto_test/v4.4.50_live_link_cycle_reconfirm/`.
- Reboot restore evidence: member log shows `[tx] reboot`, then `[team-nv] load role=0`, `[team-nv] restore member leader_suffix=279A leader=154 ret=0`, `HELLO 241->154`, `CONFIG 154->241`, `ACK 154->241`, and `[team] joined member=241`.
- Manual leave/rejoin evidence: member log shows `[tx] leave`, `[team-nv] clear web config`, then only after manual `role member 279A` it rejoins; leader log shows `member offline id=241` followed by fresh `HELLO/CONFIG/ACK/joined`.

## Hardware Boundary

- Requirements 1 and 2 have live COM13/COM16 evidence in `versions/v4.4.48/MANIFEST.md`.
- Requirement 3 has automated 30-node simulation evidence in `versions/v4.4.49/MANIFEST.md`.
- `v4.4.50` adds the missing three-board live-test entry point. Full live relay proof still requires a third board connected as leader + relay + child and a run of `--with-relay-cycle`.
