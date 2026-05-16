# V3 文档入口（手机定位桥接 + SLE 位置分发）

`docs/v3/` 记录从 V2 自动组网演进到 V3 的第一阶段能力：

- 手机浏览器定位采集（WebUI）
- WS63 HTTP 定位入口
- 位置通过 `POS_REPORT` 进入 SLE 组网
- 节点位置状态回传到 `/api/nodes`

## 当前里程碑

- `v3.0.0-alpha4`：修复 member parent timeout 轻量切换中 HELLO 发送失败导致的悬挂态，改为失败保留旧 parent 并自动重试。
- `v3.0.0-alpha3`：故障恢复优化（relay 掉线即时重平衡 + member parent 健康超时切换）与参数调优。
- `v3.0.0-alpha2`：HTTPS 局域网定位可用性增强 + HTTP 参数解析加固 + WebUI 定位入口收敛。
- `v3.0.0-alpha1`：手机定位到 SLE 位置广播链路打通。

## 版本管理

V3 起使用如下发布约束：

1. 每个版本必须包含 `versions/<version>/VERSION.md` 与 `MANIFEST.md`。
2. 每次发布必须更新 `versions/README.md` 顶部“当前版本”列表。
3. 每次发布至少执行一轮可复现验证（协议测试 / WebUI 测试 / 构建）。
