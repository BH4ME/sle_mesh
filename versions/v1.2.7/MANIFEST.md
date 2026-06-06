# v1.2.7 Manifest

本版本记录 WS63 板端 WiFi WebUI 连接可靠性修复。

主要变更：

- `xc/ws63_team_network/src/ws63_team_network_app.c`
  - 为 accepted HTTP client socket 设置 `SO_RCVTIMEO` 和 `SO_SNDTIMEO`。
  - 超时时间为 1200ms，避免手机浏览器半开连接长期占住单连接 HTTP worker。
  - HTTP backlog 从 2 提到 4，兼容手机浏览器短时间并发请求。
  - 增加 Web 角色切换队列，HTTP 请求只负责排队并重定向，SLE 初始化由 `TeamNetworkTask` 执行。
  - `/pairing` 未配置页面显示 `ready` / `starting SLE`，避免用户误判按钮无效。
  - member 初次 HELLO 的 NOT_READY 不再让角色配置流程失败，后续周期 tick 继续重试。
  - SLE RX 回调打印 `node packet role=... len=... ret=...`，辅助定位灯亮但 Web 状态不更新的问题。
- `webui/shared/console-pages.json`
  - 板端版本标识更新为 `ssr=v5`。
- `xc/ws63_team_network/src/ws63_console_pages.h`
  - 由共享配置同步生成 `ssr=v5`。
- `README.md`、`xc/ws63_team_network/README.md`
  - 补充 HTTP 连接超时说明。

验证：

```sh
UBUNTU_HOST=192.168.6.130 UBUNTU_USER=owen UBUNTU_PASS='<set locally, do not commit secrets>' \
UBUNTU_SDK=/home/owen/workspace/bearpi-pico_h3863 BUILD_JOBS=4 \
scripts/ws63_build_team_ubuntu.sh unified
```

结果：

- ws63-liteos-app build success
- packet success
- unified firmware copied back to local output path
- SRAM 34.39%，PROGRAM 55.73%
