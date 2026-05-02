# SLE Team WebUI

这是 `sle_mesh` 的第一版 Web 控制台，参考 Meshtastic Web 和 MeshCore Web App 的产品形态，但数据模型和接口按本项目协议实现。

## 目标

- 同一份静态文件可以放到 WS63 板端 HTTP 服务里。
- 同一份静态文件也可以部署到域名上，当上位机使用。
- UI 通过 HTTP API 读取节点、消息和状态，不直接绑定某一种传输方式。

## 页面

- 总览：队伍状态、节点列表、消息流、发送测试包
- 数据包：输入十六进制包，按 `Mesh Packet -> GROUP_DATA -> App Packet` 解码
- 设置：板端部署和域名部署的接口说明

## 构建

```sh
npm install
npm run build
```

构建产物在：

```text
webui/dist/
```

当前产物很小，适合作为嵌入式静态资源：

- JS gzip 约 6.4 KB
- CSS gzip 约 1.7 KB

## 运行

本地运行：

```sh
npm run dev
```

打开：

```text
http://localhost:5173/
```

默认不会显示假数据。进入“连接/设置”页或总览顶部可以切换：

- WiFi：填写任意一块带 HTTP API 的 WS63 地址，如 `http://192.168.43.1`
- 串口：选择 WebSerial 串口，默认 `115200`

域名上位机模式：

```text
https://sleweb.mecho.top/
https://sleweb.mecho.top/?api=http://192.168.43.1
```

如果页面和 API 同源，直接打开页面即可，不需要 `api` 参数。

注意：`https://sleweb.mecho.top` 是 HTTPS 页面。浏览器可能默认拦截它直接访问 `http://192.168.43.1` 这种私网 HTTP API。需要 WiFi 直连板端时，优先使用板端页面 `http://192.168.43.1/`；域名上位机更适合串口/WebSerial、后续 HTTPS 代理，或允许本地 HTTP 访问的浏览器环境。

WiFi 入口不限定 leader。两种方式都可以落地：

- leader 开 WiFi：WebUI 能看到 leader 汇总后的全队信息。
- member 开 WiFi：WebUI 连接这个 member，能看这个 member 自身状态；如果固件把收到的配置、ACK、最近 leader 信息也缓存出来，也可以显示它看到的网络视角。

## WS63 HTTP API 草案

### GET /api/status

```json
{
  "teamId": 1,
  "selfId": 1,
  "leaderId": 1,
  "role": "leader",
  "state": "online",
  "joined": true,
  "nextSeq": 67,
  "uptimeS": 205,
  "transport": "ws63-http"
}
```

### GET /api/nodes

```json
[
  {
    "id": 2,
    "role": "member",
    "online": true,
    "batteryPercent": 88,
    "fixStatus": 1,
    "lastRssiDbm": -43,
    "lastSeq": 57,
    "lastSeenS": 205,
    "latitudeE6": 39908456,
    "longitudeE6": 116397128,
    "speedCms": 100,
    "headingDeg": 90,
    "satCount": 9
  }
]
```

### GET /api/events

```json
[
  {
    "id": "evt-pos-57",
    "time": "2026-05-02T02:20:00.000Z",
    "direction": "rx",
    "type": "POS_REPORT",
    "srcId": 2,
    "dstId": 1,
    "seq": 57,
    "summary": "pos lat=39.908456 lon=116.397128 battery=88 fix=1 sat=9"
  }
]
```

### POST /api/send

当前 WS63 leader 固件已落地 `GET /api/status`、`GET /api/nodes`、`GET /api/events`。`POST /api/send` 是 WebUI 已预留的下一步接口。

```json
{
  "type": "position",
  "dstId": 1,
  "latitudeE6": 39908456,
  "longitudeE6": 116397128,
  "speedCms": 100,
  "headingDeg": 90,
  "batteryPercent": 88,
  "fixStatus": 1,
  "satCount": 9
}
```

响应返回刚产生的事件：

```json
{
  "id": "evt-tx-68",
  "time": "2026-05-02T02:20:02.000Z",
  "direction": "tx",
  "type": "POS_REPORT",
  "srcId": 1,
  "dstId": 1,
  "seq": 68,
  "summary": "position sent to 1"
}
```

## 下一步

- 在 WS63 固件里加一个轻量 HTTP server。
- 把 `g_team_node` 和最近事件环形缓冲区导出为上述 JSON。
- 把 `POST /api/send` 转成已有的 `sle_team_node_send_*()` 调用。
- 如果板端资源空间紧张，可以把 `dist` 里的文件 gzip 后内嵌。
