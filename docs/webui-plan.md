# WebUI 接入计划

## 参考

Meshtastic Web 的核心形态：

- Web Client 可以托管，也可以由节点直接提供
- UI 和设备传输层解耦
- 页面围绕节点、消息、地图、设置和包/日志展开

MeshCore 的核心形态：

- Companion / Web App / Flasher 分工
- 设备通过 BLE、USB、WiFi 等方式给客户端提供连接
- 上位机负责查看节点、消息、配置和固件工具

本项目第一版采用相同方向，但实现保持轻量：

- 不引入 protobuf
- 不直接复制 GPL Web 代码
- 不绑定 WebSerial/BLE
- 先使用 HTTP JSON API

## 两种部署形态

### 1. WS63 板端 WebUI

任意 WS63 节点作为 WiFi AP 或 STA 后提供 HTTP：

```text
http://192.168.43.1/
http://192.168.43.1/api/status
http://192.168.43.1/api/nodes
http://192.168.43.1/api/events
http://192.168.43.1/api/send
```

浏览器直接连板子查看数据和节点。leader 可以提供全队聚合视图；member 也可以提供自身视图和它缓存到的网络视图。

### 2. 域名上位机

把 `webui/dist` 部署到域名：

```text
https://console.example.com/
```

通过参数指定设备 API：

```text
https://console.example.com/?api=http://192.168.43.1
```

后续也可以让域名后端代理多个设备。

## 固件端需要补的接口

### 状态快照

从 `g_team_node` 导出：

- teamId
- selfId
- leaderId
- role
- state
- joined
- nextSeq
- uptimeS

### 节点快照

从 `g_team_node.members` 导出：

- member_id
- role
- online
- battery_percent
- fix_status
- last_rssi_dbm
- last_seq
- last_seen_s

位置数据目前只在 position callback 里打印，下一步要在 app 层保存最近一次 POS：

- latitudeE6
- longitudeE6
- speedCms
- headingDeg
- satCount

### 事件环形缓冲区

新增一个小 ring buffer，保存最近 32 条事件：

- joined
- position
- alert
- send result
- decode error

WebUI 的消息流从这里读。

### 发送接口

`POST /api/send` 映射到：

- `sle_team_node_send_heartbeat()`
- `sle_team_node_send_position()`
- `sle_team_node_send_alert()`
- `sle_team_node_send_config()`

## 当前 WebUI 已完成

- Vite + TypeScript 静态 WebUI
- mock transport
- HTTP transport 抽象
- 节点表
- 消息流
- 测试发送表单
- Mesh/GROUP_DATA/App Packet 十六进制解析器
- 静态构建通过
