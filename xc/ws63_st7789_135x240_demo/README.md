# WS63 ST7789 135x240 1.14 寸 TFT 动画 Demo

这个目录是独立的 WS63 / BearPi-Pico H3863 样例，不改现有主工程。默认使用 SPI0 写 ST7789，`CS`、`RS/DC`、`RESET` 走 GPIO 手动控制。

## 连线

```text
TFT VCC      -> WS63 3V3
TFT GND      -> WS63 GND

TFT SCL      -> WS63 GPIO7   / SPI_CLK / SCK
TFT SDA      -> WS63 GPIO9   / SPI_DO  / MOSI
TFT CS       -> WS63 GPIO8   / 片选 GPIO
TFT RS       -> WS63 GPIO10  / DC / 数据命令选择
TFT RESET    -> WS63 GPIO6   / 复位 GPIO
```

## Demo 行为

- 初始化 ST7789 为 RGB565 / 135x240。
- 上电后先显示全屏彩条，方便确认排线、颜色和偏移。
- 随后循环播放动态渐变背景、扫描线和弹跳色块动画。

## 接入 BearPi SDK

把本目录复制到 SDK：

```sh
cp -R xc/ws63_st7789_135x240_demo \
  /Users/bh4me_macair/Documents/Codex/bearpi-pico_h3863/application/samples/products/ws63_st7789_135x240_demo
```

然后在 SDK 的 `application/samples/products/CMakeLists.txt` 加：

```cmake
if(DEFINED CONFIG_SAMPLE_SUPPORT_WS63_ST7789_135X240)
    add_subdirectory_if_exist(ws63_st7789_135x240_demo)
endif()
```

在 SDK 的 `application/samples/products/Kconfig` 加一个入口：

```kconfig
config SAMPLE_SUPPORT_WS63_ST7789_135X240
    bool "Support WS63 ST7789 135x240 TFT animation demo"
    default n
    depends on ENABLE_PRODUCTS_SAMPLE

if SAMPLE_SUPPORT_WS63_ST7789_135X240
menu "WS63 ST7789 135x240 TFT Demo Configuration"
    osource "application/samples/products/ws63_st7789_135x240_demo/Kconfig"
endmenu
endif
```

菜单里打开 `SAMPLE_SUPPORT_WS63_ST7789_135X240` 后编译烧录。

## 可调参数

- `CONFIG_ST7789_SPI_FREQ_MHZ`：默认 16 MHz，飞线较长时可降到 8 或 4。
- `CONFIG_ST7789_TFT_X_OFFSET` / `CONFIG_ST7789_TFT_Y_OFFSET`：默认 `52` / `40`。如果画面偏移，优先改这两个。
- `CONFIG_ST7789_COLOR_ORDER_BGR`：如果红蓝反了，切换这个选项。
- `CONFIG_ST7789_INVERSION_ON`：部分 IPS 小屏需要开启反显，默认开启。

## 排查

- 屏幕全黑：先确认 `VCC` 是 3V3、`GND` 共地，`RESET` 接 GPIO6。
- 串口有 `[st7789] init ok` 但屏幕不动：检查 `SCL GPIO7`、`SDA GPIO9`、`CS GPIO8`、`RS GPIO10`。
- 画面有内容但错位：调 `X_OFFSET` / `Y_OFFSET`。
- 颜色怪：切换 `COLOR_ORDER_BGR` 或 `INVERSION_ON`。
