# Version v2.0.0-alpha16

版本定位：

- V2 第十六次增量（alpha）：同步烧录固件里的 C 端 SSR WebUI 与域名 Vite WebUI，让二者使用同一套 WS63 HTTP 控制语义。

本版完成：

1. 域名 WebUI 同步板端控制接口：
- 支持未配置设备的 `set leader` / `set member`；
- 支持 leader 配队、pending member 审批；
- 支持 member 选择 leader、leave、factory reset；
- WiFi 模式不再暴露未落地的 HTTP `POST /api/send` 表单。

2. 路由/中继观测同步：
- 域名 WebUI 展示 `relayAllowed/relayEnabled/relayTier/maxDownstream`；
- leader 展示 `routeMetrics` 收敛、relay online、route update 与 reparent 统计；
- 节点列表显示 MAC suffix 与中继层级。

3. API 契约与跨域动作修正：
- 新增 `webui/shared/ws63-api.json` 记录当前板端 HTTP 路由；
- WebUI 测试校验契约路由已在烧录固件中实现；
- 板端 redirect 动作响应补充 `Access-Control-Allow-Origin: *`，避免域名 WebUI 跨域调用 GET 控制接口后被浏览器拦截。

4. 固件事件时间同步：
- `/api/events` 的 `time` 字段按固件启动秒数显示；
- 域名 WebUI 将纯数字事件时间渲染为 `Xs`，避免误当 ISO 时间解析。
