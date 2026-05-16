# Version v3.0.0-alpha5

版本定位：

- 在 `v3.0.0-alpha4` 稳定基线上，处理 UI 可观测性与内存细节优化：补齐串口模式 factory reset 引导，并精简 route hint/连接跟踪冗余状态字段。

本版完成：

1. 串口模式 `factory reset` 交互修正：
- Overview 控制面板下，串口模式时 `Factory reset` 按钮禁用；
- 同位置显示明确提示：串口模式不支持 `factory reset`，请切换到 WiFi API 并调用 `/api/factory-reset`；
- 避免用户点击后才看到错误，减少“静默失败”感知。

2. route hint 状态字段精简：
- 移除 `routeHintLastActivityS` 在固件 runtime / Web API / WebUI type 的传递链路；
- 保留 `sent/failed/cooldown-skipped` 三个计数，去掉低价值时间戳输出；
- 同步更新状态 JSON 与 README 示例。

3. 连接跟踪冗余字段精简：
- 移除 `team_conn_track_t` 与 `team_pending_conn_t` 中未参与决策路径的 `bucket` 字段；
- 移除 runtime 中未被读取的 `parent_selected_id` 字段；
- 保持既有 parent 选择、route hint、failover 行为不变。

4. 部署提示补充：
- 在 `webui/README.md` 增加“非隔离网络部署后轮换外部门户 URL”的运维提示。
