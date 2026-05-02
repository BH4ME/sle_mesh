import {
  Battery,
  CircleDot,
  Compass,
  Cpu,
  Plug,
  Radio,
  RefreshCw,
  Send,
  TerminalSquare,
  Wifi,
  createElement,
  type IconNode,
} from "lucide";
import {
  createTeamApi,
  loadConnectionConfig,
  requestSerialPort,
  saveConnectionConfig,
  type ConnectionConfig,
} from "./api/client";
import { decodePacketHex, formatCoordinate } from "./protocol/codec";
import type { AllowMembersCommand, SendCommand, TeamEvent, TeamNode, TeamStatus } from "./protocol/types";
import consolePages from "../shared/console-pages.json";
import "./styles/app.css";

const hostedConsoleUrl = consolePages.hostedConsoleUrl;
const defaultDeviceApiUrl = consolePages.defaultDeviceApiUrl;

let api = createTeamApi();

interface AppState {
  status?: TeamStatus;
  nodes: TeamNode[];
  events: TeamEvent[];
  selectedTab: "overview" | "packets" | "settings";
  busy: boolean;
  error?: string;
  decodedHex: string;
  connection: ConnectionConfig;
}

const state: AppState = {
  nodes: [],
  events: [],
  selectedTab: "overview",
  busy: false,
  connection: loadConnectionConfig(),
  decodedHex:
    "1A 00 11 00 00 03 00 01 00 01 02 01 01 10 00 68 F4 60 02 48 14 F0 06 78 00 5A 00 58 01 09 00",
};

const appElement = document.querySelector<HTMLDivElement>("#app");
if (!appElement) {
  throw new Error("missing #app");
}
const root = appElement;

function icon(node: IconNode, size = 18): string {
  const svg = createElement(node);
  svg.setAttribute("aria-hidden", "true");
  svg.setAttribute("width", String(size));
  svg.setAttribute("height", String(size));
  return svg.outerHTML;
}

function roleLabel(role?: string): string {
  return role === "leader" ? "Leader" : "Member";
}

function stateLabel(value?: string): string {
  if (value === "online") return "Online";
  if (value === "joining") return "Joining";
  if (value === "discovering") return "Discovering";
  return "Idle";
}

function percent(value: number): string {
  return `${Math.max(0, Math.min(100, value))}%`;
}

function escapeHtml(value: string): string {
  return value
    .replace(/&/g, "&amp;")
    .replace(/</g, "&lt;")
    .replace(/>/g, "&gt;")
    .replace(/"/g, "&quot;");
}

function renderShell(): void {
  const status = state.status;
  root.innerHTML = `
    <main class="app-shell">
      <aside class="sidebar">
        <div class="brand">
          <div class="brand-mark">${icon(Radio, 24)}</div>
          <div>
            <strong>SLE Team</strong>
            <span>${escapeHtml(consolePages.product)}</span>
          </div>
        </div>
        <nav class="nav" aria-label="Primary">
          ${navButton("overview", "总览", CircleDot)}
          ${navButton("packets", "数据包", TerminalSquare)}
          ${navButton("settings", "连接/设置", Compass)}
        </nav>
        <section class="side-status">
          <div class="label">Transport</div>
          <div class="transport">${icon(state.connection.mode === "serial" ? Plug : Wifi, 16)}${connectionLabel()}</div>
          <div class="mini-grid">
            <span>Team</span><strong>${status?.teamId ?? "--"}</strong>
            <span>Self</span><strong>${status?.selfId ?? "--"}</strong>
            <span>State</span><strong>${stateLabel(status?.state)}</strong>
          </div>
        </section>
      </aside>
      <section class="workspace">
        <header class="topbar">
          <div>
            <h1>${status ? `Team ${status.teamId} ${roleLabel(status.role)}` : "Team Console"}</h1>
            <p>${status ? `self=${status.selfId} leader=${status.leaderId} seq=${status.nextSeq}` : connectionHint()}</p>
          </div>
          <button class="icon-button" data-action="refresh" title="刷新">
            ${icon(RefreshCw, 18)}
          </button>
        </header>
        ${state.error ? `<div class="error">${escapeHtml(state.error)}</div>` : ""}
        ${state.selectedTab === "overview" ? renderOverview() : ""}
        ${state.selectedTab === "packets" ? renderPackets() : ""}
        ${state.selectedTab === "settings" ? renderSettings() : ""}
      </section>
    </main>
  `;
  bindEvents();
}

function navButton(tab: AppState["selectedTab"], text: string, ico: IconNode): string {
  const active = state.selectedTab === tab ? "active" : "";
  return `<button class="nav-item ${active}" data-tab="${tab}">${icon(ico, 18)}<span>${text}</span></button>`;
}

function connectionLabel(): string {
  if (state.connection.mode === "wifi") return state.connection.apiBase || "未连接 WiFi API";
  if (state.connection.mode === "serial") return `Serial ${state.connection.serialBaud}`;
  return "未连接";
}

function connectionHint(): string {
  if (state.connection.mode === "wifi") return state.connection.apiBase ? `connecting to ${state.connection.apiBase}` : "先配置 WiFi API 地址或选择串口";
  return "通过浏览器 WebSerial 连接板子串口";
}

function renderOverview(): string {
  return `
    ${renderConnectionPanel("compact")}
    <section class="summary-grid">
      ${metric("节点", String(state.nodes.length), Cpu)}
      ${metric("在线", String(state.nodes.filter((node) => node.online).length), Wifi)}
      ${metric("消息", String(state.events.length), TerminalSquare)}
      ${metric("入网", state.status?.joined ? "Yes" : "No", CircleDot)}
    </section>
    <section class="two-column">
      <div class="panel">
        <div class="panel-head">
          <h2>节点</h2>
          <button class="text-button" data-action="refresh">刷新</button>
        </div>
        <div class="node-list">${state.nodes.map(renderNode).join("")}</div>
      </div>
      <div class="panel">
        <div class="panel-head">
          <h2>发送</h2>
        </div>
        ${renderSendForm()}
      </div>
    </section>
    <section class="panel">
      <div class="panel-head">
        <h2>消息</h2>
      </div>
      <div class="event-list">${state.events.map(renderEvent).join("")}</div>
    </section>
  `;
}

function metric(label: string, value: string, ico: IconNode): string {
  return `
    <div class="metric">
      <div>${icon(ico, 18)}<span>${label}</span></div>
      <strong>${escapeHtml(value)}</strong>
    </div>
  `;
}

function renderNode(node: TeamNode): string {
  const location =
    node.latitudeE6 !== undefined && node.longitudeE6 !== undefined
      ? `${formatCoordinate(node.latitudeE6)}, ${formatCoordinate(node.longitudeE6)}`
      : "no position";
  return `
    <article class="node-row">
      <div class="node-main">
        <span class="node-id">#${node.id}</span>
        <div>
          <strong>${roleLabel(node.role)}</strong>
          <span>${location}</span>
        </div>
      </div>
      <div class="node-stats">
        <span>${icon(Battery, 15)}${percent(node.batteryPercent)}</span>
        <span>${node.lastRssiDbm} dBm</span>
        <span>seq ${node.lastSeq}</span>
        <span class="${node.online ? "online" : "offline"}">${node.online ? "online" : "offline"}</span>
      </div>
    </article>
  `;
}

function renderSendForm(): string {
  return `
    <form class="send-form" data-form="send">
      <label>类型
        <select name="type">
          <option value="heartbeat">HEARTBEAT</option>
          <option value="position">POS_REPORT</option>
          <option value="alert">ALERT</option>
          <option value="config">CONFIG</option>
        </select>
      </label>
      <div class="form-grid">
        <label>Dst<input name="dstId" type="number" min="1" max="255" value="1" /></label>
        <label>电量<input name="batteryPercent" type="number" min="0" max="100" value="88" /></label>
        <label>RSSI<input name="rssiDbm" type="number" min="-128" max="127" value="-43" /></label>
        <label>Fix<input name="fixStatus" type="number" min="0" max="255" value="1" /></label>
        <label>LatE6<input name="latitudeE6" type="number" value="39908456" /></label>
        <label>LonE6<input name="longitudeE6" type="number" value="116397128" /></label>
        <label>速度<input name="speedCms" type="number" min="0" value="100" /></label>
        <label>航向<input name="headingDeg" type="number" min="0" value="90" /></label>
      </div>
      <button class="primary-button" type="submit">${icon(Send, 17)}发送</button>
    </form>
  `;
}

function renderEvent(event: TeamEvent): string {
  const labels: Record<TeamEvent["direction"], string> = {
    rx: "SLE RX",
    tx: "SLE TX",
    fail: "失败",
    cli: "串口",
    state: "状态",
    system: "系统",
  };
  return `
    <article class="event-row">
      <div class="event-type ${event.direction}">${labels[event.direction]}</div>
      <div>
        <strong>${event.type}</strong>
        <span>${escapeHtml(event.summary)}</span>
      </div>
      <time>${new Date(event.time).toLocaleTimeString()}</time>
    </article>
  `;
}

function renderPackets(): string {
  const decoded = decodePacketHex(state.decodedHex);
  return `
    <section class="panel packet-workbench">
      <div class="panel-head">
        <h2>包解析</h2>
      </div>
      <textarea class="hex-input" data-hex-input spellcheck="false">${escapeHtml(state.decodedHex)}</textarea>
      <div class="decode-result">
        ${
          decoded.ok
            ? `
              <div class="decode-grid">
                <span>Mesh</span><strong>v${decoded.mesh?.version} payload=${decoded.mesh?.payloadType} route=${decoded.mesh?.routeType}</strong>
                <span>Channel</span><strong>${decoded.mesh?.channelHash ?? "--"}</strong>
                <span>App</span><strong>${decoded.app?.type ?? "--"} seq=${decoded.app?.seq ?? "--"} ${decoded.app?.srcId ?? "--"}→${decoded.app?.dstId ?? "--"}</strong>
                <span>Body</span><pre>${escapeHtml(JSON.stringify(decoded.app?.body ?? {}, null, 2))}</pre>
              </div>
            `
            : `<div class="error">${escapeHtml(decoded.error ?? "decode failed")}</div>`
        }
      </div>
    </section>
  `;
}

function renderSettings(): string {
  return `
    ${renderConnectionPanel("full")}
    ${renderAllowPanel()}
    <section class="panel settings-panel">
      <div class="panel-head">
        <h2>部署</h2>
      </div>
      <div class="deploy-grid">
        <div>
          <strong>WS63 板端</strong>
          <span>把 dist 静态文件烧进资源分区或文件系统，由板端 HTTP 服务提供。</span>
          <code>/api/status /api/nodes /api/events /api/send</code>
        </div>
        <div>
          <strong>域名上位机</strong>
          <span>部署在 sleweb.mecho.top，串口连接可直接使用；WiFi API 直连私网设备时需要浏览器允许 HTTPS 页面访问本地 HTTP 地址。</span>
          <code>${hostedConsoleUrl}/?api=${defaultDeviceApiUrl}</code>
        </div>
      </div>
    </section>
    <section class="panel">
      <div class="panel-head">
        <h2>接口草案</h2>
      </div>
      <pre class="api-spec">GET  /api/status
GET  /api/nodes
GET  /api/events
POST /api/send  (下一步接入)
{
  "type": "position",
  "dstId": 1,
  "latitudeE6": 39908456,
  "longitudeE6": 116397128,
  "batteryPercent": 88
}</pre>
    </section>
  `;
}

function renderAllowPanel(): string {
  const status = state.status;
  const allowMode = status?.memberFilterEnabled ? "only" : "all";
  const allowMembers = status?.allowedMembers?.length
    ? status.allowedMembers.join(" ")
    : status?.allowedMemberCount
      ? `${status.allowedMemberCount} 个成员，串口刷新可展开 ID`
      : "";
  const serialOnly = state.connection.mode === "serial" ? "" : `<div class="note">成员准入写入目前走板子串口 CLI；WiFi HTTP 页面先做状态查看。</div>`;
  return `
    <section class="panel settings-panel">
      <div class="panel-head">
        <h2>成员准入</h2>
        <span class="mode-badge">${allowMode === "all" ? "allow all" : "allow only"}</span>
      </div>
      <div class="allow-summary">
        <div><span>Team</span><strong>${status?.teamId ?? "--"}</strong></div>
        <div><span>Leader</span><strong>${status?.leaderId ?? "--"}</strong></div>
        <div><span>Self</span><strong>${status?.selfId ?? "--"}</strong></div>
        <div><span>Allowed</span><strong>${escapeHtml(allowMembers || "all")}</strong></div>
      </div>
      <form class="allow-form" data-form="allow">
        <div class="segmented" role="tablist" aria-label="Member allow mode">
          <label class="segment"><input type="radio" name="allowMode" value="all" ${allowMode === "all" ? "checked" : ""} /><span>全部成员</span></label>
          <label class="segment"><input type="radio" name="allowMode" value="only" ${allowMode === "only" ? "checked" : ""} /><span>只允许列表</span></label>
        </div>
        <label>Member ID 列表
          <input name="memberIds" type="text" inputmode="numeric" placeholder="例如 2 或 2 3 4" value="${escapeHtml(status?.allowedMembers?.join(" ") ?? "")}" />
        </label>
        <div class="connection-actions">
          <button class="primary-button" type="submit">${icon(Plug, 17)}应用准入</button>
          <button class="text-button" type="button" data-action="allow-add">添加</button>
          <button class="text-button" type="button" data-action="allow-del">删除</button>
        </div>
        ${serialOnly}
      </form>
    </section>
  `;
}

function renderConnectionPanel(layout: "compact" | "full"): string {
  const title = layout === "compact" ? "当前连接" : "连接配置";
  return `
    <section class="panel settings-panel connection-panel">
      <div class="panel-head">
        <h2>${title}</h2>
        <span class="mode-badge">${connectionLabel()}</span>
      </div>
      <form class="connection-form ${layout === "compact" ? "compact" : ""}" data-form="connection">
        <div class="segmented" role="tablist" aria-label="Connection mode">
          ${connectionOption("wifi", "WiFi API")}
          ${connectionOption("serial", "串口")}
        </div>
        <div class="connection-fields">
          <label>WS63 HTTP API 地址
            <input name="apiBase" type="url" placeholder="${defaultDeviceApiUrl} 或 http://member-ip" value="${escapeHtml(state.connection.apiBase)}" />
          </label>
          <label>串口波特率
            <input name="serialBaud" type="number" min="9600" max="921600" value="${state.connection.serialBaud}" />
          </label>
        </div>
        <div class="connection-actions">
          <button class="primary-button" type="submit">${icon(Plug, 17)}保存并重连</button>
          <button class="text-button" type="button" data-action="serial-connect">选择串口</button>
        </div>
        <div class="note">
          当前是 <strong>${connectionLabel()}</strong>。WiFi 可以连接任意一块带 HTTP API 的 WS63，leader 或 member 都可以；串口是浏览器直接连接一块板子的 UART CLI。域名上位机地址是 <strong>${hostedConsoleUrl}</strong>。
        </div>
      </form>
    </section>
  `;
}

function connectionOption(mode: ConnectionConfig["mode"], label: string): string {
  const checked = state.connection.mode === mode ? "checked" : "";
  return `<label class="segment"><input type="radio" name="mode" value="${mode}" ${checked} /><span>${label}</span></label>`;
}

function readNumber(form: FormData, key: string): number | undefined {
  const value = form.get(key);
  if (typeof value !== "string" || value.trim() === "") return undefined;
  return Number(value);
}

function readMemberIds(raw: string): number[] {
  return raw
    .split(/[\s,，]+/)
    .map((part) => part.trim())
    .filter(Boolean)
    .map(Number)
    .filter((value) => Number.isInteger(value) && value >= 1 && value <= 254);
}

async function applyAllow(command: AllowMembersCommand): Promise<void> {
  try {
    const eventResult = await api.configureAllow(command);
    state.events.unshift(eventResult);
    state.error = undefined;
    await refresh();
  } catch (error) {
    state.error = error instanceof Error ? error.message : "allow config failed";
    renderShell();
  }
}

async function refresh(): Promise<void> {
  if (state.busy) {
    return;
  }
  if (state.connection.mode === "wifi" && state.connection.apiBase === "") {
    state.status = undefined;
    state.nodes = [];
    state.events = [];
    state.error = undefined;
    renderShell();
    return;
  }
  state.busy = true;
  state.error = undefined;
  renderShell();
  try {
    if (state.connection.mode === "serial") {
      state.status = await api.getStatus();
      state.nodes = await api.getNodes();
      state.events = await api.getEvents();
    } else {
      const [status, nodes, events] = await Promise.all([api.getStatus(), api.getNodes(), api.getEvents()]);
      state.status = status;
      state.nodes = nodes;
      state.events = events;
    }
  } catch (error) {
    state.error = error instanceof Error ? error.message : "refresh failed";
  } finally {
    state.busy = false;
    renderShell();
  }
}

function bindEvents(): void {
  document.querySelectorAll<HTMLButtonElement>("[data-tab]").forEach((button) => {
    button.addEventListener("click", () => {
      state.selectedTab = button.dataset.tab as AppState["selectedTab"];
      renderShell();
    });
  });
  document.querySelectorAll<HTMLButtonElement>("[data-action='refresh']").forEach((button) => {
    button.addEventListener("click", () => void refresh());
  });
  document.querySelector<HTMLTextAreaElement>("[data-hex-input]")?.addEventListener("input", (event) => {
    state.decodedHex = (event.target as HTMLTextAreaElement).value;
    renderShell();
  });
  document.querySelector<HTMLFormElement>("[data-form='send']")?.addEventListener("submit", (event) => {
    event.preventDefault();
    const formElement = event.currentTarget;
    if (!(formElement instanceof HTMLFormElement)) return;
    const form = new FormData(formElement);
    const command: SendCommand = {
      type: String(form.get("type")) as SendCommand["type"],
      dstId: readNumber(form, "dstId") ?? 1,
      batteryPercent: readNumber(form, "batteryPercent"),
      rssiDbm: readNumber(form, "rssiDbm"),
      fixStatus: readNumber(form, "fixStatus"),
      latitudeE6: readNumber(form, "latitudeE6"),
      longitudeE6: readNumber(form, "longitudeE6"),
      speedCms: readNumber(form, "speedCms"),
      headingDeg: readNumber(form, "headingDeg"),
    };
    void api
      .send(command)
      .then((eventResult) => {
        state.events.unshift(eventResult);
        return refresh();
      })
      .catch((error) => {
        state.error = error instanceof Error ? error.message : "send failed";
        renderShell();
      });
  });
  document.querySelector<HTMLFormElement>("[data-form='connection']")?.addEventListener("submit", (event) => {
    event.preventDefault();
    const formElement = event.currentTarget;
    if (!(formElement instanceof HTMLFormElement)) return;
    const form = new FormData(formElement);
    const nextConfig: ConnectionConfig = {
      mode: String(form.get("mode")) as ConnectionConfig["mode"],
      apiBase: String(form.get("apiBase") ?? "").trim().replace(/\/$/, ""),
      serialBaud: readNumber(form, "serialBaud") ?? 115200,
    };
    if (nextConfig.mode === "wifi" && nextConfig.apiBase === "") {
      state.error = `WiFi 模式需要填写 WS63 HTTP API 地址，例如 ${defaultDeviceApiUrl}`;
      renderShell();
      return;
    }
    saveConnectionConfig(nextConfig);
    state.connection = nextConfig;
    api = createTeamApi();
    state.error = undefined;
    void refresh();
  });
  document.querySelector<HTMLFormElement>("[data-form='allow']")?.addEventListener("submit", (event) => {
    event.preventDefault();
    const formElement = event.currentTarget;
    if (!(formElement instanceof HTMLFormElement)) return;
    const form = new FormData(formElement);
    const mode = String(form.get("allowMode")) === "only" ? "only" : "all";
    const memberIds = readMemberIds(String(form.get("memberIds") ?? ""));
    void applyAllow({ mode, memberIds });
  });
  document.querySelector<HTMLButtonElement>("[data-action='allow-add']")?.addEventListener("click", () => {
    const input = document.querySelector<HTMLInputElement>("input[name='memberIds']");
    const memberIds = readMemberIds(input?.value ?? "");
    void applyAllow({ mode: "add", memberIds: memberIds.slice(0, 1) });
  });
  document.querySelector<HTMLButtonElement>("[data-action='allow-del']")?.addEventListener("click", () => {
    const input = document.querySelector<HTMLInputElement>("input[name='memberIds']");
    const memberIds = readMemberIds(input?.value ?? "");
    void applyAllow({ mode: "del", memberIds: memberIds.slice(0, 1) });
  });
  document.querySelector<HTMLButtonElement>("[data-action='serial-connect']")?.addEventListener("click", () => {
    void connectSerialPlaceholder();
  });
}

async function connectSerialPlaceholder(): Promise<void> {
  const serialNavigator = navigator as Navigator & {
    serial?: {
      requestPort(options?: unknown): Promise<unknown>;
    };
  };
  if (!serialNavigator.serial) {
    state.error = "当前浏览器不支持 WebSerial。Chrome/Edge 可用，Safari 暂不支持。";
    renderShell();
    return;
  }
  try {
    await requestSerialPort(state.connection.serialBaud);
    state.error = undefined;
    state.connection = { ...state.connection, mode: "serial" };
    saveConnectionConfig(state.connection);
    api = createTeamApi();
    await refresh();
    return;
  } catch (error) {
    state.error = error instanceof Error ? error.message : "serial selection cancelled";
  }
  renderShell();
}

renderShell();
void refresh();
window.setInterval(() => {
  if (state.connection.mode === "wifi") {
    void refresh();
  }
}, 5000);
window.setInterval(() => {
  if (state.connection.mode === "serial") {
    void refresh();
  }
}, 15000);
