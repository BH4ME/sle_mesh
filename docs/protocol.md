# 早期协议草案

> 注意：这份文档是早期讨论导游语音和通用消息时留下的草案，不是当前代码正在使用的协议。
>
> 当前实现请以 [protocol/README.md](/Users/bh4me_macair/Documents/Codex/sle_intercom/docs/protocol/README.md) 和 [current_packet_layout_explained.md](/Users/bh4me_macair/Documents/Codex/sle_intercom/docs/current_packet_layout_explained.md) 为准。

下面这版不是最终协议，而是适合 MVP 的轻量协议。

## 1. 基础帧头

```text
0      1      2      3      4      5      6      7
+------+------+------+------+------+------+------+------+
| magic       | ver  | type | group_id     | seq        |
+------+------+------+------+------+------+------+------+
| timestamp_ms (32bit)                               |
+------+------+------+------+------+------+------+------+
| payload_len  | flags | hdr_crc | payload ...       |
+------+------+------+------+------+------+------+------+
```

建议字段：

- `magic`: 固定头，便于快速丢弃脏数据
- `ver`: 协议版本
- `type`: 消息类型
- `group_id`: 导游组编号
- `seq`: 包序号
- `timestamp_ms`: 发送时间戳
- `payload_len`: 载荷长度
- `flags`: 编码方式、重传标志、是否关键帧等

## 2. 消息类型

建议先定义这些：

- `0x01` `JOIN_REQ`
- `0x02` `JOIN_ACK`
- `0x03` `LEAVE_REQ`
- `0x04` `PTT_START`
- `0x05` `PTT_STOP`
- `0x06` `VOICE_DATA`
- `0x07` `HEARTBEAT`
- `0x08` `STATUS_REPORT`

## 3. 入组流程

```text
Listener -> JOIN_REQ
Guide    -> JOIN_ACK
Listener -> HEARTBEAT
Guide    -> optional STATUS/CONFIG
```

`JOIN_ACK` 中建议下发：

- `group_id`
- 当前语音编码类型
- 帧时长
- 目标缓冲深度

## 4. PTT 流程

```text
Guide key down -> PTT_START
Guide stream   -> VOICE_DATA(seq++)
Guide key up   -> PTT_STOP
```

收听端逻辑：

- 收到 `PTT_START` 清空旧播放上下文
- 收到 `VOICE_DATA` 进入抖动缓存
- 收到 `PTT_STOP` 播放完缓冲后静音

## 5. 语音载荷建议

先按人声优化：

- 编码：`IMA-ADPCM` 优先
- 帧长：`20 ms`
- 每帧独立解码

这样做的好处：

- 丢一帧只影响一个短时间窗口
- 不需要复杂参考帧恢复

## 6. 接收端缓冲

建议初始策略：

- 最小启动缓冲：`60 ms`
- 正常工作缓冲：`60 ~ 100 ms`
- 连续丢包超过阈值则插入静音帧

## 7. 丢包处理

MVP 阶段建议只做轻量补偿：

- 单包丢失：复制上一帧尾部或插静音
- 多包连续丢失：静音并等待新帧同步
- 不做复杂重传

语音实时业务里，晚到包通常比丢包更没价值。

## 8. 设备状态上报

`STATUS_REPORT` 可带：

- 电量
- RSSI
- 丢包率
- 当前缓冲深度
- 固件版本

这样后面你做导游机管理台会比较容易。
