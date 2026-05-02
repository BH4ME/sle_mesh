export type NodeRole = "leader" | "member";
export type NodeState = "idle" | "discovering" | "joining" | "online";

export type AppMessageType =
  | "HELLO"
  | "HEARTBEAT"
  | "POS_REPORT"
  | "ALERT"
  | "CONFIG"
  | "ACK"
  | "UNKNOWN";

export interface TeamStatus {
  teamId: number;
  selfId: number;
  leaderId: number;
  role: NodeRole;
  state: NodeState;
  joined: boolean;
  nextSeq: number;
  uptimeS: number;
  transport: "ws63-http" | "hosted-http" | "serial";
}

export interface TeamNode {
  id: number;
  role: NodeRole;
  online: boolean;
  batteryPercent: number;
  fixStatus: number;
  lastRssiDbm: number;
  lastSeq: number;
  lastSeenS: number;
  latitudeE6?: number;
  longitudeE6?: number;
  speedCms?: number;
  headingDeg?: number;
  satCount?: number;
}

export interface TeamEvent {
  id: string;
  time: string;
  direction: "rx" | "tx" | "system";
  type: AppMessageType;
  srcId?: number;
  dstId?: number;
  seq?: number;
  summary: string;
  rawHex?: string;
}

export interface PacketDecodeResult {
  ok: boolean;
  error?: string;
  mesh?: {
    version: number;
    payloadType: number;
    routeType: number;
    pathHashSize: number;
    hopCount: number;
    payloadLen: number;
    channelHash?: number;
    cipherMac?: string;
  };
  app?: {
    type: AppMessageType;
    typeValue: number;
    flags: number;
    seq: number;
    teamId: number;
    srcId: number;
    dstId: number;
    ttl: number;
    bodyLen: number;
    body: Record<string, number>;
  };
}

export interface SendCommand {
  type: "heartbeat" | "position" | "alert" | "config";
  dstId: number;
  batteryPercent?: number;
  rssiDbm?: number;
  fixStatus?: number;
  latitudeE6?: number;
  longitudeE6?: number;
  speedCms?: number;
  headingDeg?: number;
  satCount?: number;
  lostMemberId?: number;
  reason?: number;
  lastReportS?: number;
}
