# 手机 WebUI 定位上报到 SLE 计划书（Systematic Debugging）

## 目标

在不改动 `POS_REPORT` 协议结构的前提下，打通这条链路：

`手机浏览器定位 -> WS63 HTTP API -> sle_team_node_send_position -> SLE 广播/单播`

并在 WebUI 节点列表中看到上报后的坐标。

## Phase 1: Root Cause Investigation（先找根因）

### 现状证据

1. 协议层已经支持 `POS_REPORT`：
   - `include/sle_team_packet.h` 有 `sle_team_pos_body_t`
   - `src/sle_team_node.c` 有 `sle_team_node_send_position`
2. WS63 HTTP 路由缺口：
   - 现有 `GET /api/status|nodes|events|pending|role|pairing|member/select|member/leave|factory-reset`
   - 没有可直接触发 `send_position` 的 HTTP 路由
3. WebUI 缺口：
   - WiFi 模式显示“固件没有 /api/send”，发送表单被禁用
   - 没有 `navigator.geolocation` 采集手机定位入口
4. 数据回显缺口：
   - `TeamNode` 类型有 `latitudeE6/longitudeE6`
   - 但 `member_record` 未持久化坐标，`/api/nodes` 也未输出坐标

### 根因结论

不是协议问题，而是 **端到端接口缺失**：HTTP 入口缺失 + WebUI 采集缺失 + 节点坐标存储/输出缺失。

## Phase 2: Pattern Analysis（对照现有模式）

### 可复用模式

1. 固件现有 HTTP 控制模式：`GET + query`，响应 JSON 或 303 redirect。
2. 发送链路现有模式：路由处理函数里调用 `sle_team_node_*`，由 `team_sle_send` 统一发包。
3. WebUI 现有模式：
   - API 层 `TeamApi` 抽象
   - UI 层在 `main.ts` 绑定 form/button 事件，调用 API 并 `refresh()`

### 与目标差异

1. 缺 `GET /api/location?...`
2. 缺 `TeamApi.sendLocation(...)`
3. 缺 WebUI 手机定位按钮和错误处理
4. 缺 `member` 位置字段存储与 `/api/nodes` 输出

## Phase 3: Hypothesis and Testing（假设与验证）

### 假设

如果补齐 `api/location -> send_position -> node record -> nodes json`，并在 WebUI 接入 `geolocation`，那么手机定位可以通过 SLE 发出，且在节点列表可见。

### 最小验证（先红）

1. `webui/tests/ws63-api-contract.test.mjs` 先新增断言：
   - 合同包含 `/api/location`
   - 固件源码出现 `GET /api/location`
2. 新增源码级断言：
   - `sle_team_web_api.c` 输出 `latitudeE6/longitudeE6`
3. 新增 WebUI 断言：
   - `main.ts` 包含 `navigator.geolocation`
   - `api/client.ts` 包含 `/api/location`

## Phase 4: Implementation（单一目标实现）

1. 固件：
   - 增加 query 解析函数（有符号 `int32`、无符号 `uint16`）
   - 新增 `GET /api/location` 路由
   - 解析 `lat/lon/dst/speed/heading/battery/fix/sat`
   - 调用 `sle_team_node_send_position`
   - 返回标准事件 JSON（成功/失败）
2. 协议节点状态：
   - `sle_team_member_record_t` 增加经纬度/速度/航向/卫星数
   - 在 `sle_team_handle_position` 写入这些字段
   - `sle_team_web_write_nodes_json` 输出这些字段
3. WebUI：
   - `TeamApi` 增加 `sendLocation`
   - `HttpTeamApi` 调 `/api/location`
   - 新增“手机定位并发送”面板与按钮
   - 通过 `navigator.geolocation.getCurrentPosition` 获取坐标并发送
4. 文档与合同：
   - 更新 `webui/shared/ws63-api.json`
   - 更新 `webui/README.md` API 列表

## 验收标准

1. `npm --prefix webui test` 通过。
2. WiFi 模式下，点击“手机定位并发送”后，`/api/events` 出现 `POS_REPORT` 发包事件。
3. 对端收到位置包后，`/api/nodes` 输出对应 `latitudeE6/longitudeE6`。
4. 失败场景有明确错误：未配置角色、定位权限拒绝、参数非法、SLE 未就绪。
