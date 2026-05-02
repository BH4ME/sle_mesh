# 版本整理

当前版本：

- `v1.2.4`

## 版本目录

版本记录放在：

- [versions/](/Users/bh4me_macair/Documents/Codex/sle_intercom/versions)

当前版本目录：

- [versions/v1.2.4/](/Users/bh4me_macair/Documents/Codex/sle_intercom/versions/v1.2.4)

## v1.2.4 内容

当前版本包含：

- MeshCore 风格外层包骨架
- `GROUP_DATA` 包装层
- 明文 `App Packet`
- `HELLO / HEARTBEAT / POS_REPORT / ALERT / CONFIG / ACK`
- `leader/member` 组网状态机
- 串口 CLI 控制
- leader 成员准入白名单
- member 侧 leader 源 ID 过滤

## 版本规则建议

- `v1.2.x`：协议字段和基础组网小修
- `v1.3.x`：接入真实 HiSpark SLE 收发接口
- `v1.4.x`：加入 GNSS 和离队判断
- `v2.0.x`：加入真实加密或多跳路由

## 当前兼容性说明

`v1.2.4` 不保证与 MeshCore 官方节点直接互通。

它采用的是：

- 外层参考 MeshCore 风格
- 内层为当前项目自定义业务协议
