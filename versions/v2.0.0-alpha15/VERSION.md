# Version v2.0.0-alpha15

版本定位：

- V2 第十五次增量（alpha）：补齐 relay failover 仿真套件与场景化脚本入口，覆盖 relay 掉线补选、重连、失败回滚、边界抖动与时间回绕等高风险场景。

本版完成：

1. relay failover 专用仿真套件：
- 新增 `examples/relay_failover_suite.c`；
- 覆盖单 relay 掉线、双 relay 掉线、发送失败回滚、pairing/rebalance 竞态、阈值抖动、时间回绕六类场景。

2. 仿真脚本场景化：
- `scripts/simulate_v2.sh` 支持 `--suite=core|failover|all`；
- 可按需运行核心协议回归或 failover 回归，日志分开输出。

3. 回归验证增强：
- `simulate_v2.sh --suite=failover` 与 `simulate_v2.sh --suite=core` 均可独立通过；
- `simulate_20_members.sh` 保持 20-member 逻辑回归通过。
