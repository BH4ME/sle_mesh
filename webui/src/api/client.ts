import type { SendCommand, TeamEvent, TeamNode, TeamStatus } from "../protocol/types";

export interface TeamApi {
  getStatus(): Promise<TeamStatus>;
  getNodes(): Promise<TeamNode[]>;
  getEvents(): Promise<TeamEvent[]>;
  send(command: SendCommand): Promise<TeamEvent>;
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
  const response = await fetch(path, {
    headers: { "content-type": "application/json" },
    ...init,
  });
  if (!response.ok) {
    throw new Error(`${response.status} ${response.statusText}`);
  }
  return response.json() as Promise<T>;
}

export class HttpTeamApi implements TeamApi {
  constructor(private readonly baseUrl = "") {}

  getStatus(): Promise<TeamStatus> {
    return fetchJson(`${this.baseUrl}/api/status`);
  }

  getNodes(): Promise<TeamNode[]> {
    return fetchJson(`${this.baseUrl}/api/nodes`);
  }

  getEvents(): Promise<TeamEvent[]> {
    return fetchJson(`${this.baseUrl}/api/events`);
  }

  send(command: SendCommand): Promise<TeamEvent> {
    return fetchJson(`${this.baseUrl}/api/send`, {
      method: "POST",
      body: JSON.stringify(command),
    });
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
  return {
    teamId: Number(match[1]),
    selfId: Number(match[2]),
    leaderId: Number(match[3]),
    role: Number(match[4]) === 1 ? "leader" : "member",
    state: state === 3 ? "online" : state === 2 ? "joining" : state === 1 ? "discovering" : "idle",
    joined: Number(match[6]) !== 0,
    nextSeq: Number(match[7]),
    uptimeS: 0,
    transport: "serial",
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
    lastRssiDbm: Number(match[6]),
    lastSeq: Number(match[7]),
    lastSeenS: Number(match[8]),
  };
}

function sendCommandToCli(command: SendCommand): string {
  if (command.type === "heartbeat") {
    return `hb ${command.dstId} ${command.batteryPercent ?? 100} ${command.rssiDbm ?? -50} ${command.fixStatus ?? 1}`;
  }
  if (command.type === "position") {
    return `pos ${command.dstId} ${command.latitudeE6 ?? 0} ${command.longitudeE6 ?? 0} ${command.speedCms ?? 0} ${command.headingDeg ?? 0} ${command.batteryPercent ?? 100} ${command.fixStatus ?? 0} ${command.satCount ?? 0}`;
  }
  if (command.type === "alert") {
    return `alert ${command.dstId} ${command.lostMemberId ?? 0} ${command.reason ?? 0} ${command.latitudeE6 ?? 0} ${command.longitudeE6 ?? 0} ${command.lastReportS ?? 0}`;
  }
  return `config ${command.dstId}`;
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
    this.lastLines = [...this.lastLines, ...lines].slice(-40);
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
      .map((line, index) => ({
        id: `serial-line-${index}`,
        time: new Date().toISOString(),
        direction: line.includes("[team]") || line.includes("[cli]") ? "rx" : "system",
        type: line.includes("pos") ? "POS_REPORT" : line.includes("alert") ? "ALERT" : "UNKNOWN",
        summary: line,
      }));
  }

  async send(command: SendCommand): Promise<TeamEvent> {
    const cli = sendCommandToCli(command);
    const lines = await this.runCli(cli, 700);
    return {
      id: `serial-${Date.now()}`,
      time: new Date().toISOString(),
      direction: "tx",
      type:
        command.type === "heartbeat"
          ? "HEARTBEAT"
          : command.type === "position"
            ? "POS_REPORT"
            : command.type === "alert"
              ? "ALERT"
              : "CONFIG",
      dstId: command.dstId,
      summary: lines.find((line) => line.includes("[cli]")) ?? cli,
    };
  }
}

export function createTeamApi(): TeamApi {
  const config = loadConnectionConfig();
  if (config.mode === "serial") {
    return new SerialTeamApi(config.serialBaud);
  }
  return new HttpTeamApi(config.apiBase);
}
