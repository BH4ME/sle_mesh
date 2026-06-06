# v4.2.3 Manifest

## 变更范围

- `xc/ws63_team_network/src/ws63_team_network_app.c`
- `xc/ws63_team_network/Kconfig`
- `scripts/ws63_build_v4_ubuntu.sh`
- `xc/ws63_team_network/README.md`
- `docs/v4/README.md`
- `README.md`
- `versions/README.md`
- `versions/v4.2.3/VERSION.md`
- `versions/v4.2.3/MANIFEST.md`

## 关键改动

1. 版本与硬件档位
- 固件版本号更新为 `v4.2.3`。
- 去掉旧的 `v4.1` 约束文案歧义，统一到 `v4.2.3 board map`。

2. 四项映射固化
- 显示(ST7789)：`IO6/7/8/9/13`
- 蜂鸣器：`IO14`
- WS 灯：`IO0`
- GPS：`UART1 + IO17/IO18`

3. GPS 可观测性
- 新增 GPS Kconfig 映射项（enable / bus / tx / rx）。
- 启动日志输出 GPS bus/pin 信息，现场能直接看串口确认映射是否生效。

4. 构建脚本同步
- Ubuntu 远端构建脚本同步 `v4.2.3` 档位文案。
- 构建时写入 GPS 映射配置项；下载电路相关行为不改。

## 验证

```sh
rg -n "SLE_TEAM_FW_VERSION|CONFIG_SLE_TEAM_GPS_UART|CONFIG_SLE_TEAM_ST7789_|CONFIG_SLE_TEAM_WS2812_PIN|CONFIG_SLE_TEAM_BUZZER_PIN" \
  xc/ws63_team_network/src/ws63_team_network_app.c \
  xc/ws63_team_network/Kconfig \
  scripts/ws63_build_v4_ubuntu.sh
```
