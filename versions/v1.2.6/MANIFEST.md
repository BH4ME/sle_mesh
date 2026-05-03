# v1.2.6 Manifest

本版本记录 WS63 WebUI 配置持久化和恢复出厂。

主要变更：

- `xc/ws63_team_network/src/ws63_team_network_app.c`
  - 增加 NV flash 配置结构，保存 WebUI 设置的 role/team/leader suffix/channel。
  - 启动时读取并校验 NV 配置，自动恢复 leader/member。
  - Web 角色设置成功后写入 NV，并调用 `uapi_nv_flush()`。
  - `/api/factory-reset` 清除 Web 配置并 watchdog 重启。
  - member `leave` 成功后清除 Web 配置。
  - `/pairing` 页面取消自动刷新，避免手机填表被中断。
- `webui/shared/console-pages.json`
  - 板端版本标识更新为 `ssr=v4`。
- `xc/ws63_team_network/src/ws63_console_pages.h`
  - 由共享配置同步生成 `ssr=v4`。
- `README.md`、`xc/ws63_team_network/README.md`
  - 补充 NV flash 持久化、恢复出厂和运行时状态边界。

验证：

```sh
UBUNTU_HOST=192.168.6.130 UBUNTU_USER=owen UBUNTU_PASS='67215837' \
UBUNTU_SDK=/home/owen/workspace/bearpi-pico_h3863 BUILD_JOBS=4 \
scripts/ws63_build_team_ubuntu.sh unified
```

结果：

- ws63-liteos-app build success
- packet success
- unified firmware copied back to local output path
