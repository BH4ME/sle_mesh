import type {
  AllowMembersCommand,
  MemberSelectCommand,
  PairingCommand,
  PendingMember,
  RoleCommand,
  SendCommand,
  TeamEvent,
  TeamNode,
  TeamStatus,
  UnconfiguredStatus,
} from "../protocol/types";

export interface TeamApi {
  getStatus(): Promise<TeamStatus | UnconfiguredStatus>;
  getNodes(): Promise<TeamNode[]>;
  getEvents(): Promise<TeamEvent[]>;
  getPending(): Promise<PendingMember[]>;
  configureRole(command: RoleCommand): Promise<void>;
  configurePairing(command: PairingCommand): Promise<void>;
  selectMemberLeader(command: MemberSelectCommand): Promise<void>;
  leaveMember(): Promise<void>;
  factoryReset(): Promise<void>;
  send(command: SendCommand): Promise<TeamEvent>;
  configureAllow(command: AllowMembersCommand): Promise<TeamEvent>;
}

export type ConnectionMode = "wifi" | "serial";

export interface ConnectionConfig {
  mode: ConnectionMode;
  apiBase: string;
  serialBaud: number;
}

const defaultConfig: ConnectionConfig = {
  mode: "wifi",
  apiBase: "",
  serialBaud: 115200,
};

const configKey = "sle-team-connection";

export function loadConnectionConfig(): ConnectionConfig {
  const url = new URL(window.location.href);
  const api = url.searchParams.get("api");
  if (api !== null) {
    return { ...defaultConfig, mode: "wifi", apiBase: api };
  }

  try {
    const raw = window.localStorage.getItem(configKey);
    if (!raw) return defaultConfig;
    const parsed = JSON.parse(raw) as Partial<ConnectionConfig>;
    return {
      mode: parsed.mode === "serial" ? "serial" : "wifi",
      apiBase: typeof parsed.apiBase === "string" ? parsed.apiBase : "",
      serialBaud: typeof parsed.serialBaud === "number" ? parsed.serialBaud : 115200,
    };
  } catch {
    return defaultConfig;
  }
}

export function saveConnectionConfig(config: ConnectionConfig): void {
  window.localStorage.setItem(configKey, JSON.stringify(config));
}

async function fetchJson<T>(path: string, init?: RequestInit): Promise<T> {
  const requestInit = init?.body
    ? { headers: { "content-type": "application/json" }, ...init }
    : init;
  const response = await fetch(path, requestInit);
  if (!response.ok) {
    throw new Error(`${response.status} ${response.statusText}`);
  }
  return response.json() as Promise<T>;
}

async function fetchAction(path: string): Promise<void> {
  const response = await fetch(path, { redirect: "manual" });
  if (response.ok || response.type === "opaqueredirect" || (response.status >= 300 && response.status < 400)) {
    return;
  }
  throw new Error(`${response.status} ${response.statusText}`);
}

function qs(params: Record<string, string | number | boolean>): string {
  const out = new URLSearchParams();
  for (const [key, value] of Object.entries(params)) {
    out.set(key, String(value));
  }
  return out.toString();
}

export class HttpTeamApi implements TeamApi {
  constructor(private readonly baseUrl = "") {}

  getStatus(): Promise<TeamStatus | UnconfiguredStatus> {
    return fetchJson(`${this.baseUrl}/api/status`);
  }

  getNodes(): Promise<TeamNode[]> {
    return fetchJson(`${this.baseUrl}/api/nodes`);
  }

  getEvents(): Promise<TeamEvent[]> {
    return fetchJson(`${this.baseUrl}/api/events`);
  }

  getPending(): Promise<PendingMember[]> {
    return fetchJson(`${this.baseUrl}/api/pending`);
  }

  configureRole(command: RoleCommand): Promise<void> {
    if (command.role === "leader") {
      return fetchAction(`${this.baseUrl}/api/role?role=leader`);
    }
    return fetchAction(
      `${this.baseUrl}/api/role?${qs({
        role: "member",
        leader: command.leaderSuffix,
        team: command.teamId,
        channel: command.channel,
      })}`,
    );
  }

  configurePairing(command: PairingCommand): Promise<void> {
    if (command.action === "approve") {
      return fetchAction(
        `${this.baseUrl}/api/pairing?${qs({
          action: "approve",
          id: command.id,
          relay: command.relay ? 1 : 0,
        })}`,
      );
    }
    return fetchAction(`${this.baseUrl}/api/pairing?action=${command.action}`);
  }

  selectMemberLeader(command: MemberSelectCommand): Promise<void> {
    return fetchAction(
      `${this.baseUrl}/api/member/select?${qs({
        team: command.teamId,
        leader: command.leaderSuffix,
        channel: command.channel,
      })}`,
    );
  }

  leaveMember(): Promise<void> {
    return fetchAction(`${this.baseUrl}/api/member/leave`);
  }

  factoryReset(): Promise<void> {
    return fetchAction(`${this.baseUrl}/api/factory-reset`);
  }

  send(command: SendCommand): Promise<TeamEvent> {
    return fetchJson(`${this.baseUrl}/api/send`, {
      method: "POST",
      body: JSON.stringify(command),
    });
  }

  configureAllow(_command: AllowMembersCommand): Promise<TeamEvent> {
    return Promise.reject(new Error("WiFi HTTP 暂未提供配置写入接口，请切到串口模式执行成员准入配置"));
  }
}

type SerialPortLike = {
  readable: ReadableStream<Uint8Array> | null;
  writable: WritableStream<Uint8Array> | null;
  open(options: { baudRate: number }): Promise<void>;
};

type SerialNavigator = Navigator & {
  serial?: {
    requestPort(options?: unknown): Promise<SerialPortLike>;
  };
};

let selectedSerialPort: SerialPortLike | undefined;
let serialQueue: Promise<unknown> = Promise.resolve();

export async function requestSerialPort(baudRate: number): Promise<void> {
  const serialNavigator = navigator as SerialNavigator;
  if (!serialNavigator.serial) {
    throw new Error("当前浏览器不支持 WebSerial。请用 Chrome 或 Edge，并通过 HTTPS 打开页面。");
  }
  selectedSerialPort = await serialNavigator.serial.requestPort();
  await selectedSerialPort.open({ baudRate });
}

function cliStateToStatus(line: string): TeamStatus | undefined {
  const match = line.match(
    /team=(\d+)\s+self=(\d+)\s+leader=(\d+)\s+role=(\d+)\s+state=(\d+)\s+joined=(\d+)\s+seq=(\d+)/,
  );
  if (!match) return undefined;
  const state = Number(match[5]);
  const pairingMatch = line.match(/pairing=(\d+)/);
  const allowMatch = line.match(/allow=(all|only)\s+allow_count=(\d+)/);
  return {
    configured: true,
    teamId: Number(match[1]),
    selfId: Number(match[2]),
    leaderId: Number(match[3]),
    role: Number(match[4]) === 1 ? "leader" : "member",
    state: state === 3 ? "online" : state === 2 ? "joining" : state === 1 ? "discovering" : "idle",
    joined: Number(match[6]) !== 0,
    nextSeq: Number(match[7]),
    uptimeS: 0,
    transport: "serial",
    pairingEnabled: pairingMatch ? Number(pairingMatch[1]) !== 0 : undefined,
    memberFilterEnabled: allowMatch?.[1] === "only",
    allowedMemberCount: allowMatch ? Number(allowMatch[2]) : 0,
    allowedMembers: [],
  };
}

function cliMemberToNode(line: string): TeamNode | undefined {
  const match = line.match(
    /member=(\d+)\s+role=(\d+)\s+online=(\d+)\s+battery=(\d+)\s+fix=(\d+)\s+rssi=(-?\d+)\s+last_seq=(\d+)\s+last_seen=(\d+)/,
  );
  if (!match) return undefined;
  return {
    id: Number(match[1]),
    role: Number(match[2]) === 1 ? "leader" : "member",
    online: Number(match[3]) !== 0,
    batteryPercent: Number(match[4]),
    fixStatus: Number(match[5]),
    lastRssiDbm: Number(match[6]) === 127 ? null : Number(match[6]),
    lastSeq: Number(match[7]),
    lastSeenS: Number(match[8]),
  };
}

function cliPendingToMember(line: string): PendingMember | undefined {
  const match = line.match(/pending member=(\d+)\s+role=(\d+)\s+battery=(\d+)\s+mac=([0-9A-Fa-f]{4})\s+ready=(\d+)\s+last_seen=(\d+)/);
  if (!match) return undefined;
  return {
    id: Number(match[1]),
    role: Number(match[2]) === 1 ? "leader" : "member",
    batteryPercent: Number(match[3]),
    macReady: Number(match[5]) !== 0,
    macSuffix: match[4].toUpperCase(),
    lastSeenS: Number(match[6]),
  };
}

function routeIdFromSuffix(suffix: string): number {
  const value = Number.parseInt(suffix, 16) & 0xffff;
  let routeId = value & 0xff;
  if (routeId === 0 || routeId === 0xff) {
    routeId = ((value >> 8) % 254) + 1;
  }
  return routeId;
}

function sendCommandToCli(command: SendCommand): string {
  if (command.type === "heartbeat") {
    return `hb ${command.dstId} ${command.batteryPercent ?? 100} ${command.rssiDbm ?? 127} ${command.fixStatus ?? 1}`;
  }
  if (command.type === "position") {
    return `pos ${command.dstId} ${command.latitudeE6 ?? 0} ${command.longitudeE6 ?? 0} ${command.speedCms ?? 0} ${command.headingDeg ?? 0} ${command.batteryPercent ?? 100} ${command.fixStatus ?? 0} ${command.satCount ?? 0}`;
  }
  if (command.type === "alert") {
    return `alert ${command.dstId} ${command.lostMemberId ?? 0} ${command.reason ?? 0} ${command.latitudeE6 ?? 0} ${command.longitudeE6 ?? 0} ${command.lastReportS ?? 0}`;
  }
  return `config ${command.dstId}`;
}

function allowCommandToCli(command: AllowMembersCommand): string {
  if (command.mode === "all") {
    return "allow all";
  }
  const ids = (command.memberIds ?? []).map((id) => Math.trunc(id)).filter((id) => id >= 1 && id <= 254);
  if (command.mode === "only") {
    if (ids.length === 0) throw new Error("allow only 需要至少一个 member id");
    return `allow only ${ids.join(" ")}`;
  }
  if (ids.length !== 1) throw new Error("allow add/del 每次只处理一个 member id");
  return `allow ${command.mode} ${ids[0]}`;
}

function appTypeFromText(text: string): TeamEvent["type"] {
  const upper = text.toUpperCase();
  if (upper.includes("HEARTBEAT")) return "HEARTBEAT";
  if (upper.includes("POS_REPORT") || upper.includes("POSITION") || upper.includes(" POS ")) return "POS_REPORT";
  if (upper.includes("ALERT")) return "ALERT";
  if (upper.includes("CONFIG")) return "CONFIG";
  if (upper.includes("HELLO")) return "HELLO";
  if (upper.includes("ACK")) return "ACK";
  if (upper.includes("PACKET")) return "PACKET";
  if (text.startsWith("[cli-tx]") || text.startsWith("[cli-rx]") || text.startsWith("[cli]")) return "CLI";
  if (text.startsWith("[state]") || text.startsWith("[team]")) return "STATE";
  if (text.startsWith("[team-wifi]")) return "SYSTEM";
  return "UNKNOWN";
}

function cleanTaggedLine(line: string): string {
  return line.replace(/^\[[^\]]+\]\s*/, "");
}

function serialLineToEvent(line: string, index: number): TeamEvent {
  const cleaned = cleanTaggedLine(line);
  const base = {
    id: `serial-line-${Date.now()}-${index}`,
    time: new Date().toISOString(),
    type: appTypeFromText(line),
    summary: cleaned,
  };
  if (line.startsWith("[sle-tx-ok]")) {
    return { ...base, direction: "tx", summary: `SLE发送成功：${cleaned}` };
  }
  if (line.startsWith("[sle-tx-fail]")) {
    return { ...base, direction: "fail", summary: `SLE发送失败：${cleaned}` };
  }
  if (line.startsWith("[sle-rx]")) {
    return { ...base, direction: "rx", summary: `SLE收到：${cleaned}` };
  }
  if (line.startsWith("[cli-tx]")) {
    return { ...base, direction: "cli", summary: `串口命令：${cleaned}` };
  }
  if (line.startsWith("[cli-rx]") || line.startsWith("[cli]")) {
    return { ...base, direction: "cli", summary: `串口返回：${cleaned}` };
  }
  if (line.startsWith("[state]") || line.startsWith("[team]")) {
    return { ...base, direction: "state", summary: `状态：${cleaned}` };
  }
  if (line.startsWith("[team-wifi]")) {
    return { ...base, direction: "state", summary: `WiFi状态：${cleaned}` };
  }
  return { ...base, direction: "system", summary: line };
}

export class SerialTeamApi implements TeamApi {
  private lastLines: string[] = [];

  constructor(private readonly baudRate = 115200) {}

  private async ensurePort(): Promise<SerialPortLike> {
    if (!selectedSerialPort) {
      await requestSerialPort(this.baudRate);
    }
    if (!selectedSerialPort?.readable || !selectedSerialPort.writable) {
      throw new Error("串口未打开");
    }
    return selectedSerialPort;
  }

  private async runCliUnlocked(command: string, waitMs = 450): Promise<string[]> {
    const port = await this.ensurePort();
    const writer = port.writable!.getWriter();
    try {
      this.lastLines = [...this.lastLines, `[cli-tx] ${command}`].slice(-80);
      await writer.write(new TextEncoder().encode(`${command}\r\n`));
    } finally {
      writer.releaseLock();
    }

    const reader = port.readable!.getReader();
    const lines: string[] = [];
    const deadline = Date.now() + waitMs;
    let text = "";
    try {
      while (Date.now() < deadline) {
        const timeout = new Promise<ReadableStreamReadResult<Uint8Array>>((resolve) => {
          window.setTimeout(() => resolve({ done: true, value: undefined }), 80);
        });
        const result = await Promise.race([reader.read(), timeout]);
        if (result.done) continue;
        text += new TextDecoder().decode(result.value);
      }
    } finally {
      reader.releaseLock();
    }
    text
      .split(/\r?\n/)
      .map((line) => line.trim())
      .filter(Boolean)
      .forEach((line) => lines.push(line));
    this.lastLines = [...this.lastLines, ...lines].slice(-80);
    return lines;
  }

  private runCli(command: string, waitMs = 450): Promise<string[]> {
    const task = serialQueue.then(() => this.runCliUnlocked(command, waitMs));
    serialQueue = task.catch(() => undefined);
    return task;
  }

  async getStatus(): Promise<TeamStatus> {
    const lines = await this.runCli("state");
    const status = lines.map(cliStateToStatus).find(Boolean);
    if (!status) throw new Error("串口没有返回 state，确认板子串口 CLI 已启动");
    if (status.memberFilterEnabled) {
      const allowLines = await this.runCli("allow");
      status.allowedMembers = allowLines
        .map((line) => line.match(/allow member=(\d+)/)?.[1])
        .filter((value): value is string => value !== undefined)
        .map(Number);
      status.allowedMemberCount = status.allowedMembers.length;
    }
    return status;
  }

  async getNodes(): Promise<TeamNode[]> {
    const lines = await this.runCli("members");
    return lines.map(cliMemberToNode).filter((node): node is TeamNode => node !== undefined);
  }

  async getEvents(): Promise<TeamEvent[]> {
    return this.lastLines
      .slice(-20)
      .reverse()
      .map((line, index) => serialLineToEvent(line, index));
  }

  async getPending(): Promise<PendingMember[]> {
    const lines = await this.runCli("pairing pending");
    return lines.map(cliPendingToMember).filter((member): member is PendingMember => member !== undefined);
  }

  async configureRole(command: RoleCommand): Promise<void> {
    if (command.role === "leader") {
      await this.runCli("role leader", 1200);
      return;
    }
    await this.runCli(`role member ${command.leaderSuffix}`, 1200);
  }

  async configurePairing(command: PairingCommand): Promise<void> {
    if (command.action === "approve") {
      await this.runCli(`pairing approve ${command.id} ${command.relay ? "relay" : "norelay"}`, 800);
      return;
    }
    await this.runCli(`pairing ${command.action}`, 800);
  }

  async selectMemberLeader(command: MemberSelectCommand): Promise<void> {
    await this.runCli(`join ${command.teamId} ${routeIdFromSuffix(command.leaderSuffix)} ${command.channel}`, 800);
  }

  async leaveMember(): Promise<void> {
    await this.runCli("leave", 800);
  }

  async factoryReset(): Promise<void> {
    return Promise.reject(new Error("串口模式暂无 factory reset CLI；请用 WiFi HTTP /api/factory-reset 或串口 leave 后重新配置"));
  }

  async send(command: SendCommand): Promise<TeamEvent> {
    const cli = sendCommandToCli(command);
    await this.runCli(cli, 700);
    const latest = [...this.lastLines].reverse().find((line) => line.includes("[sle-tx-")) ?? `[cli-tx] ${cli}`;
    return serialLineToEvent(latest, Date.now());
  }

  async configureAllow(command: AllowMembersCommand): Promise<TeamEvent> {
    const cli = allowCommandToCli(command);
    await this.runCli(cli, 500);
    const latest = [...this.lastLines].reverse().find((line) => line.includes("allow ")) ?? `[cli-tx] ${cli}`;
    return serialLineToEvent(latest, Date.now());
  }
}

export function createTeamApi(): TeamApi {
  const config = loadConnectionConfig();
  if (config.mode === "serial") {
    return new SerialTeamApi(config.serialBaud);
  }
  return new HttpTeamApi(config.apiBase);
}
