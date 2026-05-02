# Version v1.2.4

当前版本：

- `v1.2.4`

说明：

- 增加 leader 侧成员准入白名单，降低同 `team_id/channel_hash/leader_id` 场景下误收 member 的风险。
- 增加 member 侧 leader 源 ID 过滤，member 只接受 `src_id == leader_id` 的包。
- 串口 CLI 增加 `allow` 命令：
  - `allow`
  - `allow all`
  - `allow only 2`
  - `allow add 3`
  - `allow del 3`
- `state` 输出增加 `allow=... allow_count=...`，便于 WebUI 和人工排查。
- `/api/status` 增加 `memberFilterEnabled`、`allowedMemberCount`、`allowedMembers`。
- 域名 WebUI 设置页增加成员准入面板；当前写入路径为 WebSerial 串口 CLI，WiFi HTTP 仅能查看状态。

已验证状态：

- 本地 packet 测试通过。
- 本地 network demo 增加白名单拒绝/放行断言并通过。
- WebUI `npm run build` 通过。
- VM 内 leader/member 固件均已重新编译并 packet success。

固件记录：

- leader:
  `/Users/bh4me_macair/Documents/Codex/bearpi-pico_h3863/output_from_vm/team_network_leader_unified_sle/ws63-liteos-app_leader_all.fwpkg`
- member:
  `/Users/bh4me_macair/Documents/Codex/bearpi-pico_h3863/output_from_vm/team_network_member_unified_sle/ws63-liteos-app_member_all.fwpkg`

已知限制：

- `allow` 配置当前只保存在 RAM 中，复位或断电后恢复默认 `allow all`。
- 如果两个 leader 的 `team_id`、`leader_id`、`channel_hash` 都完全一样，member 发现阶段仍可能先连到其中一个；leader 白名单会拒绝不属于自己的 member，但还不是完整加密配对。
- `cipher_mac` 仍是占位字段，尚未做真实认证。
