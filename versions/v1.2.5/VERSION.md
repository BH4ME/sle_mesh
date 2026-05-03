# Version v1.2.5

当前版本：

- `v1.2.5`

说明：

- WS63 内置 WebUI 增加 `/pairing` 页面。
- leader 可通过内置 WebUI 或串口 CLI 执行：
  - `pairing start`
  - `pairing pending`
  - `pairing approve <id>`
  - `pairing stop`
- member 可通过内置 WebUI 或串口 CLI 执行：
  - `join <team> <leader> <channel>`
  - `leave`
- leader 配队窗口打开时，未批准 member 的 `HELLO` 会进入 pending 列表，不会直接 ACK、CONFIG 或登记到 nodes。
- leader 批准 member 后，会把 member 加入 RAM 白名单，并发送 `CONFIG + ACK`，member 收到 ACK 后进入 joined。
- `/api/status` 增加 `pairingEnabled`，新增 `/api/pending`、`/api/pairing`、`/api/member/select`、`/api/member/leave`。
- WS63 改为同一个统一固件包，开机后先处于 `unconfigured`，再通过内置 WebUI 或串口 CLI 选择 leader/member。
- SSID 和 WebUI 自标识改为 WiFi MAC 后四位：`SLE-TEAM-WS63-XXXX`、`UXXXX`、`LXXXX`、`MXXXX`。
- WebUI 配队、离队、选择 leader 等动作改为执行后自动回到 `/pairing`，避免手机浏览器停在 JSON 页面。
- HELLO 负载增加完整 MAC，leader pending/member 记录里保存完整 MAC；MAC 后四位用于显示和现场选择，1 字节 route ID 仍用于包路由。
- 现场确认小熊派 WS63 蓝色用户灯是 `GPIO2 active-high`，修正默认极性；之前按 active-low 会导致蓝灯上电常亮。
- 增加串口 LED 诊断命令：`led status/on/off/tx/rx/active_high/active_low/pin <0-31>`。

已验证状态：

- 本地 packet 测试通过。
- 本地 network demo 增加配队窗口、pending、approve、member leave/select 断言并通过。
- WebUI `npm run build` 通过。
- Ubuntu 编译机 `owen@192.168.6.130` 出包通过，统一运行时角色固件已生成。

已知限制：

- 本版本没有扩到 20 个 member，仍保持当前 `SLE_TEAM_MAX_MEMBERS`。
- 配队、角色和 member 选择仍是 RAM 运行时配置，断电或复位后回到 `unconfigured`。
- 当前不是完整“扫描附近 leader 并选择”的发现系统；member 选择 leader 仍需要填写 `team / leader / channel`。
- 当前配队不是加密认证，`cipher_mac` 仍未用于真实鉴权。
- 新 HELLO 和旧固件不兼容；leader/member 两块板都需要烧录本版统一固件。
