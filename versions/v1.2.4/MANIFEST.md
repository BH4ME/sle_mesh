# v1.2.4 Manifest

本版本记录成员准入和队伍匹配加固。

主要变更：

- `include/sle_team_node.h`
  - 增加成员准入配置字段和白名单操作 API。
- `src/sle_team_node.c`
  - leader 拒绝非白名单 member 的 `HELLO / HEARTBEAT / POS_REPORT`。
  - member 拒绝 `src_id` 不是本地 `leader_id` 的包。
  - 白名单设置先校验再生效，并自动去重。
- `src/sle_team_cli.c`
  - 增加 `allow [all|only <id...>|add <id>|del <id>]`。
  - `state` 输出增加准入状态。
- `src/sle_team_web_api.c`
  - `/api/status` 输出准入状态和白名单列表。
- `webui/`
  - 串口模式支持 WebUI 下发 `allow` 命令。
  - 设置页增加成员准入面板。
  - WiFi HTTP 模式明确为状态查看，暂不写入配置。
- `examples/team_network_demo.c`
  - 增加白名单拒绝/放行模拟断言。
- `README.md`、`webui/README.md`、`xc/ws63_team_network/README.md`
  - 补充队伍匹配、双 leader 风险、成员准入和运行时配置限制。

验证：

```sh
cc -Wall -Wextra -Werror -Iinclude -DSLE_TEAM_PACKET_TEST src/sle_team_packet.c examples/team_node_common.c -o /private/tmp/sle_team_packet_test && /private/tmp/sle_team_packet_test
cc -Wall -Wextra -Werror -Iinclude -DSLE_TEAM_NETWORK_TEST src/sle_team_packet.c src/sle_team_node.c examples/team_network_demo.c -o /private/tmp/sle_team_network_test && /private/tmp/sle_team_network_test
npm run build
scripts/ws63_build_team_vm.sh leader
scripts/ws63_build_team_vm.sh member
```

结果：

- packet test passed
- network test passed
- WebUI build passed
- leader firmware packet success
- member firmware packet success
