# 当前协议包结构说明

这份文档只解释当前仓库里**已经写出来的这版代码**，不讨论未来可能的严格 MeshCore 兼容版本。

对应代码文件：

- [include/sle_team_packet.h](/Users/bh4me_macair/Documents/Codex/sle_intercom/include/sle_team_packet.h)
- [src/sle_team_packet.c](/Users/bh4me_macair/Documents/Codex/sle_intercom/src/sle_team_packet.c)
- [examples/team_node_common.c](/Users/bh4me_macair/Documents/Codex/sle_intercom/examples/team_node_common.c)

最重要的一句话：

**当前这版没有真正加密。**

目前只有“预留给以后加密的位置”，即：

- `channel_hash`
- `cipher_mac`

但当前实现里：

- `cipher_mac` 只是占位
- `app_packet` 仍然是明文

## 1. 最终包一共有 3 层

当前发送出去的最终字节流一共分成三层：

1. `Mesh Packet` 外层
2. `GROUP_DATA` 包装层
3. `App Packet` 业务层

整体结构如下：

```text
+-------------------- Mesh Packet --------------------+
| header | transport(opt) | path_len | path | payload |
+-----------------------------------------------------+
                                                |
                                                v
+------------------- GROUP_DATA Payload -------------------+
| channel_hash(1) | cipher_mac(2) | app_packet(variable)   |
+----------------------------------------------------------+
                                                |
                                                v
+---------------------- App Packet ------------------------+
| msg_type | flags | seq | team_id | src | dst | ttl | len |
| body...                                                  |
+----------------------------------------------------------+
```

## 2. 第一层：Mesh Packet 外层

对应结构体：

- `sle_team_mesh_packet_t`

编码/解码函数：

- `sle_team_encode_mesh_packet()`
- `sle_team_decode_mesh_packet()`

## 2.1 header

第 0 个字节是 `header`。

构造逻辑：

```c
header = ((version & 0x03) << 6) |
         ((payload_type & 0x0F) << 2) |
         (route_type & 0x03);
```

位分布：

```text
bit7 bit6 | bit5 bit4 bit3 bit2 | bit1 bit0
 version  |     payload_type     | route_type
```

字段精确定义：

- `version`：2 bit
- `payload_type`：4 bit
- `route_type`：2 bit

### 示例

当前测试包第一个字节：

```text
0x1A = 00011010b
```

拆开后：

- `version = 00b = 0`
- `payload_type = 0110b = 6`
- `route_type = 10b = 2`

也就是：

- `version = SLE_TEAM_PAYLOAD_V1`
- `payload_type = SLE_TEAM_PKT_GROUP_DATA`
- `route_type = SLE_TEAM_ROUTE_DIRECT`

## 2.2 transport codes

只有下面两种 `route_type` 才会带 transport code：

- `SLE_TEAM_ROUTE_TRANSPORT_FLOOD`
- `SLE_TEAM_ROUTE_TRANSPORT_DIRECT`

如果启用，则在 `header` 后面紧跟 4 字节：

```text
transport_code_1 : 2 bytes little-endian
transport_code_2 : 2 bytes little-endian
```

编码顺序：

```text
[tc1 low][tc1 high][tc2 low][tc2 high]
```

当前示例使用的是：

- `route_type = DIRECT`

所以：

- **当前包没有 transport_code**

## 2.3 path_len

`path_len` 是紧跟在 `header` 或 transport 后面的 1 字节。

构造函数：

- `sle_team_make_path_length()`

编码逻辑：

```c
path_len = ((path_hash_size - 1) << 6) | (hop_count & 0x3F);
```

位分布：

```text
bit7 bit6 | bit5 bit4 bit3 bit2 bit1 bit0
hash_size |           hop_count
```

精确定义：

- 高 2 位：`path_hash_size - 1`
- 低 6 位：`hop_count`

当前示例里：

- `path_hash_size = 1`
- `hop_count = 0`

因此：

- `path_len = 0x00`

## 2.4 path

路径字段长度计算方式：

```c
path_bytes = path_hash_size * hop_count
```

当前示例：

- `path_hash_size = 1`
- `hop_count = 0`

所以：

- `path_bytes = 0`
- 当前包没有 path 数据

## 2.5 payload

外层剩余所有字节都属于 `payload`。

当前这版代码中，payload 实际结构是：

```text
[channel_hash][cipher_mac][app_packet]
```

## 3. 第二层：GROUP_DATA 包装层

对应函数：

- `sle_team_wrap_mesh_group_data()`
- `sle_team_unwrap_mesh_group_data()`

当前代码里，这层固定结构是：

```text
offset 0: channel_hash    1 byte
offset 1: cipher_mac[0]   1 byte
offset 2: cipher_mac[1]   1 byte
offset 3...: app_packet
```

也就是：

```text
GROUP_DATA = 3字节包装头 + App Packet
```

## 3.1 channel_hash

- 1 字节
- 当前示例设为 `0x11`

来源代码：

```c
sle_team_wrap_mesh_group_data(0x11, cipher_mac, ...)
```

## 3.2 cipher_mac

- 2 字节
- 固定放在 `payload[1]` 和 `payload[2]`

当前代码中：

```c
uint8_t cipher_mac[2] = {0x00, 0x00};
```

所以当前写入的是：

```text
00 00
```

注意：

**这不是加密结果，也不是认证标签，只是占位。**

## 3.3 app_packet

紧跟在 `payload[3]` 之后，内容为业务层包。

## 4. 第三层：App Packet 业务层

对应结构体：

- `sle_team_app_packet_t`

编码/解码函数：

- `sle_team_encode_app_packet()`
- `sle_team_decode_app_packet()`

当前格式为：

```text
offset 0  : msg_type      1 byte
offset 1  : flags         1 byte
offset 2  : seq low       1 byte
offset 3  : seq high      1 byte
offset 4  : team_id       1 byte
offset 5  : src_id        1 byte
offset 6  : dst_id        1 byte
offset 7  : ttl           1 byte
offset 8  : body_len low  1 byte
offset 9  : body_len high 1 byte
offset 10+: body
```

注意：

- `seq` 是小端
- `body_len` 也是小端

## 4.1 字段含义

### msg_type

业务消息类型。

当前已定义：

- `HELLO`
- `HEARTBEAT`
- `POS_REPORT`
- `ALERT`
- `CONFIG`
- `ACK`

### flags

扩展标志位。

当前未使用，先保留。

### seq

消息序号，用于：

- 去重
- 确认
- 调试

### team_id

队伍编号，用于多支队伍隔离。

### src_id

源节点 ID。

### dst_id

目标节点 ID。

### ttl

跳数控制。

当前单跳组网里作用不大，但为后续扩展保留。

### body_len

业务体长度。

### body

具体业务数据，不同 `msg_type` 对应不同结构。

## 5. App Body 具体结构

## 5.1 HELLO

对应结构体：

- `sle_team_hello_body_t`

格式：

```text
+-----------+------+---------+----------+
| device_id | role | battery | reserved |
+-----------+------+---------+----------+
| 1 byte    | 1    | 1       | 1        |
+-----------+------+---------+----------+
```

用途：

- 入网
- 节点身份声明

## 5.2 HEARTBEAT

对应结构体：

- `sle_team_heartbeat_body_t`

格式：

```text
+---------+----------+------------+----------+
| battery | rssi_dbm | fix_status | reserved |
+---------+----------+------------+----------+
| 1 byte  | 1        | 1          | 1        |
+---------+----------+------------+----------+
```

用途：

- 心跳保活
- 上报链路和定位状态

## 5.3 POS_REPORT

对应结构体：

- `sle_team_pos_body_t`

格式：

```text
+-------------+--------------+-----------+-------------+---------+--------+------+----------+
| latitude_e6 | longitude_e6 | speed_cms | heading_deg | battery | fix    | sat  | reserved |
+-------------+--------------+-----------+-------------+---------+--------+------+----------+
| int32       | int32        | uint16    | uint16      | 1 byte  | 1      | 1    | 1        |
+-------------+--------------+-----------+-------------+---------+--------+------+----------+
```

字段说明：

- `latitude_e6`：纬度乘 `1e6`
- `longitude_e6`：经度乘 `1e6`
- `speed_cms`：厘米每秒
- `heading_deg`：朝向角
- `battery_percent`：电量
- `fix_status`：定位状态
- `sat_count`：卫星数

## 5.4 ALERT

对应结构体：

- `sle_team_alert_body_t`

格式：

```text
+----------------+--------+----------+------------------+-------------------+---------------+
| lost_member_id | reason | reserved | last_latitude_e6 | last_longitude_e6 | last_report_s |
+----------------+--------+----------+------------------+-------------------+---------------+
| 1 byte         | 1      | 2        | int32            | int32             | uint32        |
+----------------+--------+----------+------------------+-------------------+---------------+
```

用途：

- 离队告警
- 保存最后位置和最后上报时间

## 5.5 CONFIG

对应结构体：

- `sle_team_config_body_t`

格式：

```text
+-------------------+-----------------+-----------------+----------------------+
| report_interval_s | warn_distance_m | lost_distance_m | heartbeat_timeout_s  |
+-------------------+-----------------+-----------------+----------------------+
| uint16            | uint16          | uint16          | uint16               |
+-------------------+-----------------+-----------------+----------------------+
```

用途：

- 下发队伍参数

## 5.6 ACK

对应结构体：

- `sle_team_ack_body_t`

格式：

```text
+---------+----------------+-------------+
| ack_seq | acked_msg_type | status_code |
+---------+----------------+-------------+
| uint16  | 1 byte         | 1 byte      |
+---------+----------------+-------------+
```

用途：

- 消息确认

## 6. 以当前测试包为例逐字节拆解

当前测试输出：

```text
1A 00 11 00 00 03 00 01 00 01 02 01 01 10 00 68 F4 60 02 48 14 F0 06 78 00 5A 00 58 01 09 00
```

## 6.1 外层 Mesh

```text
[0] 1A
```

- `header = 0x1A`
- `version = 0`
- `payload_type = 6 (GROUP_DATA)`
- `route_type = 2 (DIRECT)`

```text
[1] 00
```

- `path_len = 0x00`
- `path_hash_size = 1`
- `hop_count = 0`

因此：

- 没有 transport
- 没有 path

## 6.2 GROUP_DATA

```text
[2] 11
```

- `channel_hash = 0x11`

```text
[3] 00
[4] 00
```

- `cipher_mac = 0x0000`

注意：

**当前这里不是加密标签，只是占位。**

## 6.3 App Packet 头

```text
[5] 03
```

- `msg_type = POS_REPORT`

```text
[6] 00
```

- `flags = 0`

```text
[7] 01
[8] 00
```

- `seq = 1`

```text
[9] 01
```

- `team_id = 1`

```text
[10] 02
```

- `src_id = 2`

```text
[11] 01
```

- `dst_id = 1`

```text
[12] 01
```

- `ttl = 1`

```text
[13] 10
[14] 00
```

- `body_len = 16`

## 6.4 POS_REPORT Body

### latitude_e6

```text
[15] 68
[16] F4
[17] 60
[18] 02
```

小端解析：

```text
0x0260F468 = 39908456
```

即：

```text
39.908456
```

### longitude_e6

```text
[19] 48
[20] 14
[21] F0
[22] 06
```

小端解析：

```text
0x06F01448 = 116397128
```

即：

```text
116.397128
```

### speed_cms

```text
[23] 78
[24] 00
```

- `120 cm/s`

### heading_deg

```text
[25] 5A
[26] 00
```

- `90`

### battery_percent

```text
[27] 58
```

- `88`

### fix_status

```text
[28] 01
```

- 已定位

### sat_count

```text
[29] 09
```

- 9

### reserved

```text
[30] 00
```

- 保留

## 7. 当前这版到底怎么加密

准确答案：

**当前没有加密。**

当前这版代码里的相关字段只有：

```text
[channel_hash][cipher_mac][app_packet]
```

但具体实现是：

```c
packet->payload[0] = channel_hash;
packet->payload[1] = cipher_mac[0];
packet->payload[2] = cipher_mac[1];
memcpy(&packet->payload[3], app_payload, app_payload_len);
```

当前没有：

- AES
- ChaCha
- HMAC
- CMAC
- 签名
- 密钥协商

因此现在的状态是：

- `channel_hash`：逻辑信道标识
- `cipher_mac`：2 字节占位
- `app_packet`：明文复制进去

## 8. 当前逻辑有没有明显错误

从当前设计目标看，这版逻辑是自洽的，没有明显自相矛盾。

它的目标是：

- 主从共用一套协议
- 先统一包定义
- 先跑通编解码
- 先完成 leader/member 基础联调

当前已满足：

- 外层能编码/解码
- 中层能包装/拆包
- 业务层能解析
- 本地 round-trip 测试已通过

但要注意：

**这是一版“项目协议骨架”，不是“真正安全通信最终协议”。**

## 9. 如果后面要加密，应该放在哪一层

正确位置是第二层：

```text
GROUP_DATA = [channel_hash][cipher_mac][ciphertext]
```

未来真实加密流程应是：

1. 先构造 `app_packet`
2. 对 `app_packet` 做加密，得到 `ciphertext`
3. 计算认证标签，写入 `cipher_mac`
4. 再包入外层 `GROUP_DATA`

到那时：

- `app_packet` 不再明文出现
- `cipher_mac` 也不再是 `00 00`

## 10. 当前这版一句话总结

当前实现的最终包结构是：

```text
Mesh Header
+ Path Info
+ GROUP_DATA Wrapper
+ 明文 App Packet
+ 明文业务 Body
```

其中：

- 路由层已有骨架
- 业务层已有完整字节定义
- 加密层目前还没实现，只留了位置
