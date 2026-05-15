# Version v3.0.0-alpha2

版本定位：

- 在 `v3.0.0-alpha1` 手机定位桥接基线之上，补齐可用性与稳健性：HTTPS 局域网定位指引、WebUI 入口收敛、固件 HTTP 参数解析加固。

本版完成：

1. WebUI HTTPS 与定位链路可用性增强：
- Vite 配置启用 `basic-ssl`，开发/预览默认 HTTPS；
- README 增加局域网手机访问 HTTPS 的操作步骤与证书提示；
- “手机定位”入口强调 HTTPS 使用场景。

2. WebUI 交互行为修正：
- WiFi 模式显式关闭 `/api/send` 发送表单，避免误导；
- 手机定位按钮保持 `type="button"`，避免触发表单提交；
- “手机定位”面板仅在 WiFi 模式渲染，减少串口模式双入口混淆。

3. WS63 HTTP 参数解析防护：
- `team_http_query_u8/u16/i32` 增加终止符校验，仅允许数字后跟 `&` 或字符串结束；
- 拒绝携带尾随脏字符的参数输入，降低错误参数误接受风险。

4. 回归验证与合同测试补强：
- 扩展 `webui/tests/ws63-api-contract.test.mjs`，覆盖 HTTPS 脚本/配置/文档约束与 UI 行为约束；
- 通过 WebUI 测试、构建和 C 侧仿真回归。
