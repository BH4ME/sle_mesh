# v4.4.10 Manifest

## Changed Files (core of this release)

- `xc/ws63_team_network/src/ws63_team_network_app.c`
- `xc/ws63_team_network/sle_uart_client/sle_uart_client.c`
- `include/sle_team_node.h`
- `examples/team_network_demo.c`
- `examples/relay_failover_suite.c`
- `examples/relay_rebalance_demo.c`
- `scripts/simulate_20_members.sh`
- `scripts/simulate_python_1v20.sh`
- `webui/tests/ws63-api-contract.test.mjs`
- `xc/ws63_team_network/src/ws63_st7789_display.c`
- `scripts/ws63_build_v4_ubuntu.sh`
- `README.md`
- `xc/ws63_team_network/README.md`
- `versions/README.md`
- `versions/v4.4.10/VERSION.md`
- `versions/v4.4.10/MANIFEST.md`

## Key Behavior Deltas

1. Leader now rescans on partial-offline members (not only pairing/zero-conn).
2. SLE client pairing cache cleanup follows SDK-compatible reconnect flow.
3. Logical member capacity and related regression checks are aligned to 30.
4. Version anchors are synchronized to `v4.4.10`.

## Verification Checklist

Run locally:

```text
npm --prefix webui test
npm --prefix webui run build
git diff --check
```

Recommended simulation:

```text
scripts/simulate_20_members.sh
scripts/simulate_python_1v20.sh
```

Field verification focus:

1. Leader + member baseline join.
2. Reboot one member, verify leader recovers online count.
3. Drop one relay member, verify:
   - leader logs relay offline + immediate rebalance,
   - remaining members reselect/recover routes,
   - online/lost converges back as rejoin completes.
