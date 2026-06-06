# v4.4.41 Manifest

## Changed Files

- `include/sle_team_packet.h`
- `src/sle_team_node.c`
- `examples/team_node_regression_test.c`
- `examples/team_network_demo.c`
- `scripts/simulate_v2.sh`
- `xc/ws63_team_network/src/ws63_team_network_app.c`
- `scripts/ws63_build_v4_ubuntu.sh`
- `README.md`
- `versions/README.md`
- `versions/v4.4.41/VERSION.md`
- `versions/v4.4.41/MANIFEST.md`

## Key Deltas

1. Added active member leave notification:
   - `SLE_TEAM_ALERT_LEAVE`
   - member sends leave alert before clearing local state
   - leader marks the member offline immediately
2. Split semantics clearly:
   - reboot/signal loss uses disconnect/heartbeat timeout and automatic restore/rejoin
   - manual leave clears active leader and waits for manual rejoin
3. Unified firmware leave behavior:
   - WebUI `/api/member/leave`
   - serial `leave`
4. Added regression coverage for:
   - rebooted member offline then rejoin
   - manual leave, no auto-rejoin, manual rejoin
5. Updated firmware visible version to `v4.4.41`.

## Verification

Passed:

```sh
bash -lc "cc -Wall -Werror -I/path/to/sle/include /path/to/sle/examples/team_node_regression_test.c /path/to/sle/src/sle_team_packet.c /path/to/sle/src/sle_team_node.c -o /tmp/team_node_regression_test && /tmp/team_node_regression_test"
```

Result:

```text
[team-node-regression] pass
```

Passed:

```sh
bash scripts/simulate_v2.sh --suite=core --iterations=1
bash scripts/simulate_v2.sh --suite=all --iterations=1
git diff --check
```

Results:

```text
[sim] summary: pass=1 fail=0 total=1
git diff --check: no whitespace errors, CRLF warnings only
```

## Build / Flash Status

Remote Ubuntu build was attempted by SSH to `owen@192.168.6.5`, but the host rejected non-interactive authentication:

```text
Permission denied (publickey,password).
```

The v4.4.41 firmware package has not yet been built or flashed in this pass. Required live validation remains:

1. Build v4.4.41 unified firmware on the Ubuntu build host.
2. Flash COM16 and COM13.
3. Configure COM16 as leader and COM13 as member.
4. Confirm member reboot: leader sees offline, then online again without manual join.
5. Confirm manual leave: leader sees offline immediately, member does not auto-rejoin, manual rejoin succeeds.

## Notes

The existing relay failover simulations still pass through `simulate_v2.sh --suite=all`, so the earlier relay self-healing model remains covered locally. Full multi-board relay live validation is still a separate hardware test.
