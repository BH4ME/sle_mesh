#!/usr/bin/env python3
"""Build the WS63 v4 unified firmware on the LAN Ubuntu SDK host.

This is the Python/Paramiko equivalent of scripts/build/ws63_build_v4_ubuntu.sh.
It avoids depending on local ssh/rsync/WSL tooling and prints visible progress.
"""

from __future__ import annotations

import argparse
import os
import posixpath
import shutil
import sys
import tarfile
import tempfile
import time
from pathlib import Path

import paramiko


VERSION = "v4.4.126"
REMOTE_PROTO_REL = "third_party/sle_mesh"
REMOTE_APP_REL = "application/samples/products/sle_team_network"
REMOTE_PKG_REL = "output/ws63/fwpkg/ws63-liteos-app/ws63-liteos-app_all.fwpkg"
CONFIG_REL = "build/config/target_config/ws63/menuconfig/acore/ws63_liteos_app.config"
LOCAL_OUT_REL = "output_from_vm/team_network_v4_unified_runtime_role/ws63-liteos-app_v4_unified_all.fwpkg"
EXCLUDES = {".git", "build", "dist", "node_modules", "__pycache__"}


def repo_root() -> Path:
    return Path(__file__).resolve().parents[3]


def log(message: str) -> None:
    print(message, flush=True)


def quote(value: str) -> str:
    return "'" + value.replace("'", "'\"'\"'") + "'"


def add_dir_to_tar(tar: tarfile.TarFile, src: Path, arc_root: str) -> None:
    for path in src.rglob("*"):
        rel = path.relative_to(src)
        if any(part in EXCLUDES for part in rel.parts):
            continue
        arcname = str(Path(arc_root) / rel).replace("\\", "/")
        tar.add(path, arcname=arcname, recursive=False)


def make_archive(src: Path, arc_root: str, dst: Path) -> None:
    with tarfile.open(dst, "w:gz") as tar:
        add_dir_to_tar(tar, src, arc_root)


def run_remote(client: paramiko.SSHClient, command: str, stage: str, timeout: int | None = None) -> str:
    log(f"[remote] {stage}")
    stdin, stdout, stderr = client.exec_command(command, get_pty=True, timeout=timeout)
    stdin.close()
    output_parts: list[str] = []
    while not stdout.channel.exit_status_ready():
        while stdout.channel.recv_ready():
            chunk = stdout.channel.recv(4096).decode("utf-8", errors="replace")
            output_parts.append(chunk)
            print(chunk, end="", flush=True)
        time.sleep(0.1)
    while stdout.channel.recv_ready():
        chunk = stdout.channel.recv(4096).decode("utf-8", errors="replace")
        output_parts.append(chunk)
        print(chunk, end="", flush=True)
    while stderr.channel.recv_stderr_ready():
        chunk = stderr.channel.recv_stderr(4096).decode("utf-8", errors="replace")
        output_parts.append(chunk)
        print(chunk, end="", flush=True)
    status = stdout.channel.recv_exit_status()
    if status != 0:
        raise RuntimeError(f"remote stage failed ({stage}) exit={status}")
    return "".join(output_parts)


def upload_file(sftp: paramiko.SFTPClient, local: Path, remote: str) -> None:
    log(f"[upload] {local.name} -> {remote}")
    sftp.put(str(local), remote)


def download_file(sftp: paramiko.SFTPClient, remote: str, local: Path) -> None:
    local.parent.mkdir(parents=True, exist_ok=True)
    log(f"[download] {remote} -> {local}")
    sftp.get(remote, str(local))


def versioned_output_path(latest_output: Path, version: str) -> Path:
    return latest_output.with_name(f"{latest_output.stem}_{version}{latest_output.suffix}")


def reserve_unique_path(path: Path) -> Path:
    if not path.exists():
        return path
    for index in range(1, 1000):
        candidate = path.with_name(f"{path.stem}.{index}{path.suffix}")
        if not candidate.exists():
            return candidate
    raise RuntimeError(f"could not reserve unique firmware output path near {path}")


def build_config_script() -> str:
    return r'''
from pathlib import Path
import sys

path = Path(sys.argv[1])
self_id = sys.argv[2]
s = path.read_text()

def set_kconfig_value(text, key, value):
    lines = text.splitlines()
    found = False
    out = []
    for line in lines:
        if line.startswith(key + "=") or line.startswith(f"# {key} is not set"):
            if not found:
                out.append(f"{key}={value}")
                found = True
            continue
        out.append(line)
    if not found:
        out.append(f"{key}={value}")
    return "\n".join(out) + "\n"

def unset_kconfig_bool(text, key):
    lines = text.splitlines()
    found = False
    out = []
    for line in lines:
        if line.startswith(key + "=") or line.startswith(f"# {key} is not set"):
            if not found:
                out.append(f"# {key} is not set")
                found = True
            continue
        out.append(line)
    if not found:
        out.append(f"# {key} is not set")
    return "\n".join(out) + "\n"

s = set_kconfig_value(s, "CONFIG_SLE_TEAM_SELF_ID", self_id)
s = set_kconfig_value(s, "CONFIG_SAMPLE_ENABLE", "y")
s = set_kconfig_value(s, "CONFIG_SAMPLE_SUPPORT_SLE_TEAM_NETWORK", "y")
s = unset_kconfig_bool(s, "CONFIG_SAMPLE_SUPPORT_SLE_UART")
s = unset_kconfig_bool(s, "CONFIG_SAMPLE_SUPPORT_SLE_UART_SERVER")
s = unset_kconfig_bool(s, "CONFIG_SAMPLE_SUPPORT_SLE_UART_CLIENT")
s = unset_kconfig_bool(s, "CONFIG_SAMPLE_SUPPORT_SLE_UART_1_VS_8")
s = unset_kconfig_bool(s, "CONFIG_SAMPLE_SUPPORT_SLE_UART_SERVER_1_VS_8")
s = unset_kconfig_bool(s, "CONFIG_SAMPLE_SUPPORT_SLE_UART_CLIENT_1_VS_8")
s = unset_kconfig_bool(s, "CONFIG_SAMPLE_SUPPORT_BLE_UART")
s = unset_kconfig_bool(s, "CONFIG_SAMPLE_SUPPORT_SLE_GETAWAY")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_LEADER_ID", "1")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_UART_BUS", "0")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_UART_TXD_PIN", "21")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_UART_RXD_PIN", "22")
s = set_kconfig_value(s, "CONFIG_AT_UART", "3")
s = unset_kconfig_bool(s, "CONFIG_DYNAMIC_UART_ID_BINDDING")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_LED_PIN", "255")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_WS2812_ENABLE", "y")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_WS2812_PIN", "0")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_BUZZER_ENABLE", "y")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_BUZZER_PIN", "14")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_BUZZER_ACTIVE_HIGH", "y")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_BUZZER_MUTED", "y")
s = unset_kconfig_bool(s, "CONFIG_SLE_TEAM_BUZZER_AUTO_TOGGLE")
s = unset_kconfig_bool(s, "CONFIG_SLE_TEAM_GPS_ENABLE")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_GPS_UART_BUS", "1")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_GPS_UART_TXD_PIN", "17")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_GPS_UART_RXD_PIN", "18")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_ADC_ENABLE", "y")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_ADC_CTRL_PIN", "5")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_ADC_VBAT_PIN", "12")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_ADC_VBAT_CHANNEL", "5")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_ADC_CTRL_ACTIVE_HIGH", "y")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_ADC_SAMPLE_SETTLE_MS", "50")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_ADC_SAMPLE_INTERVAL_S", "30")
s = set_kconfig_value(s, "CONFIG_ADC_SUPPORT_AUTO_SCAN", "y")
s = set_kconfig_value(s, "CONFIG_ADC_USING_V154", "y")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_ST7789_ENABLE", "y")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_ST7789_SPI_BUS", "0")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_ST7789_SCLK_PIN", "7")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_ST7789_MOSI_PIN", "9")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_ST7789_CS_PIN", "8")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_ST7789_CS_ALWAYS_LOW", "y")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_ST7789_DC_PIN", "13")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_ST7789_RESET_PIN", "10")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_ST7789_X_OFFSET", "40")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_ST7789_Y_OFFSET", "53")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_ST7789_WIDTH", "240")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_ST7789_HEIGHT", "135")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_DISPLAY_USE_LVGL", "y")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_LVGL_DRAW_BUF_LINES", "8")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_HEARTBEAT_INTERVAL_S", "1")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_HEARTBEAT_TIMEOUT_S", "4")
s = set_kconfig_value(s, "CONFIG_SPI_SUPPORT_MASTER", "y")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_WIFI_AP_ENABLE", "y")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_WIFI_AP_AUTO_START", "y")
s = set_kconfig_value(s, "CONFIG_SLE_TEAM_WIFI_AP_SSID", '"SLE-TEAM-V4"')
s = set_kconfig_value(s, "CONFIG_SUPPORT_SLE_PERIPHERAL", "y")
s = set_kconfig_value(s, "CONFIG_SUPPORT_SLE_CENTRAL", "y")
path.write_text(s)
print("configured v4.4.126 v3.2 schematic pinmap, muted buzzer, boot hardware report, RGB status states, and ADC battery sampling")
'''


def post_build_guard_script() -> str:
    return r'''
from pathlib import Path
import sys

sdk = Path(sys.argv[1])
cfg_path = sdk / "build/config/target_config/ws63/menuconfig/acore/ws63_liteos_app.config"
map_path = sdk / "output/ws63/acore/ws63-liteos-app/ws63-liteos-app.map"
elf_path = sdk / "output/ws63/acore/ws63-liteos-app/ws63-liteos-app.elf"
app_source_path = sdk / "application/samples/products/sle_team_network/src/ws63_team_network_app.c"
proto_source_path = sdk / "third_party/sle_mesh/src/sle_team_node.c"

cfg = cfg_path.read_text(errors="replace")
map_text = map_path.read_text(errors="replace")
elf = elf_path.read_bytes()
app_source = app_source_path.read_text(errors="replace")
proto_source = proto_source_path.read_text(errors="replace")

required_cfg = [
    "CONFIG_SAMPLE_SUPPORT_SLE_TEAM_NETWORK=y",
    "CONFIG_SLE_TEAM_WS2812_ENABLE=y",
    "CONFIG_SLE_TEAM_WS2812_PIN=0",
    "CONFIG_SLE_TEAM_BUZZER_ENABLE=y",
    "CONFIG_SLE_TEAM_BUZZER_PIN=14",
    "CONFIG_SLE_TEAM_BUZZER_ACTIVE_HIGH=y",
    "CONFIG_SLE_TEAM_BUZZER_MUTED=y",
    "# CONFIG_SLE_TEAM_GPS_ENABLE is not set",
    "CONFIG_SLE_TEAM_ADC_ENABLE=y",
    "CONFIG_SLE_TEAM_ADC_CTRL_PIN=5",
    "CONFIG_SLE_TEAM_ADC_VBAT_PIN=12",
    "CONFIG_SLE_TEAM_ADC_VBAT_CHANNEL=5",
    "CONFIG_SLE_TEAM_ADC_SAMPLE_SETTLE_MS=50",
    "CONFIG_SLE_TEAM_ADC_SAMPLE_INTERVAL_S=30",
    "CONFIG_ADC_SUPPORT_AUTO_SCAN=y",
    "CONFIG_ADC_USING_V154=y",
    "CONFIG_SLE_TEAM_ST7789_ENABLE=y",
    "CONFIG_SLE_TEAM_ST7789_SCLK_PIN=7",
    "CONFIG_SLE_TEAM_ST7789_MOSI_PIN=9",
    "CONFIG_SLE_TEAM_ST7789_CS_PIN=8",
    "CONFIG_SLE_TEAM_ST7789_CS_ALWAYS_LOW=y",
    "CONFIG_SLE_TEAM_ST7789_DC_PIN=13",
    "CONFIG_SLE_TEAM_ST7789_RESET_PIN=10",
    "CONFIG_SLE_TEAM_DISPLAY_USE_LVGL=y",
]
for item in required_cfg:
    if item not in cfg:
        raise SystemExit(f"post-build guard failed: missing {item}")

for forbidden in [
    "CONFIG_SAMPLE_SUPPORT_SLE_UART=y",
    "CONFIG_SAMPLE_SUPPORT_SLE_UART_1_VS_8=y",
    "CONFIG_SAMPLE_SUPPORT_SLE_UART_CLIENT_1_VS_8=y",
    "CONFIG_SAMPLE_SUPPORT_SLE_UART_SERVER_1_VS_8=y",
]:
    if forbidden in cfg:
        raise SystemExit(f"post-build guard failed: official UART sample still enabled: {forbidden}")

for item in [
    "ws63_team_network_app.c.obj",
    "ws63_st7789_display.c.obj",
    "sle_team_node.c.obj",
]:
    if item not in map_text:
        raise SystemExit(f"post-build guard failed: linked map missing {item}")

for item in [
    b"v4.4.126",
    b"seek stop timeout, fallback connect pending",
    b"connect request addr:",
    b"cfg direct",
    b"runtimeDirectCap",
    b"runtimeRelayBudget",
    b"plan=%u",
    b"[display] st7789 ready",
    b"phase=%s",
    b"ready",
    b"failed",
    b"TeamDisplayTask",
    b"[display-event] event=%s label=%s member=%u",
    b"[team] boot unconfigured",
    b"[hw] init summary fw=%s",
    b"[hw] gps configured=%u present=0 ready=%u",
    b"[hw] adc present=%u ready=%u",
    b"[battery] sample valid=%u",
    b"bat commands: status|sample",
    b"vbat_mv=%u",
    b"[state] rgb state=%s",
    b"state=%s flash=%s",
    b"buzz muted by firmware",
    b"[cfg-json]",
    b"clear allowlist",
    b"scope=config+allowlist",
    b"member=%u label=%s",
    b"[team] disconnect lookup",
    b"already_offline=%u",
    b"relay failover begin",
    b"relay failover holding relay target",
    b"relay config notify pending",
    b"relay failover member=%u route pending next_hop=%u",
    b"reason=physical-parent",
    b"failover-parent-not-ready",
    b"hello ack deferred until reselect parent",
    b"config deferred until reselect parent",
    b"reason=RESELECT_PARENT",
    b"liveness preserved",
    b"direct cap prune confirmed",
    b"direct cap prune disconnect",
    b"direct cap prune deferred",
    b"parent reselect drop old leader conn",
    b"parent reselect drop upstream",
    b"seek filter reject",
    b"relay swap observe",
    b"swap-promote",
    b"swap-demote",
    b"v3.2 schematic pinmap, muted buzzer",
]:
    if item not in elf:
        raise SystemExit(f"post-build guard failed: ELF missing {item.decode('ascii', errors='replace')}")

for source_name, source_text, item in [
    ("sle_team_node.c", proto_source, "Route updates are leader policy hints"),
    ("sle_team_node.c", proto_source, "sle_team_member_has_reselect_target"),
    ("sle_team_node.c", proto_source, "hello ack deferred until reselect parent"),
    ("ws63_team_network_app.c", app_source, "team_member_relay_can_accept_child"),
    ("ws63_team_network_app.c", app_source, "team_leader_enforce_direct_capacity"),
    ("ws63_team_network_app.c", app_source, "SLE_TEAM_WS2812_BREATHE_PERIOD_MS"),
    ("ws63_team_network_app.c", app_source, "team_ws2812_start_flash(TEAM_RGB_STATE_ERROR)"),
    ("ws63_team_network_app.c", app_source, "team_ws2812_start_flash(TEAM_RGB_STATE_LEADER)"),
    ("ws63_team_network_app.c", app_source, "team_member_parent_reselect_disconnect_tick"),
    ("ws63_team_network_app.c", app_source, "RESELECT_PARENT"),
    ("ws63_team_network_app.c", app_source, "Leader-bound relayed packets always go upstream before bucket tier routing."),
    ("ws63_team_network_app.c", app_source, "team_route_next_hop_is_direct_peer"),
    ("ws63_team_network_app.c", app_source, "Direct next-hop ids use the recorded physical conn_id"),
    ("ws63_team_network_app.c", app_source, "team_leader_reconcile_online_routes"),
    ("ws63_team_network_app.c", app_source, "app_packet.src_id is the logical origin"),
    ("ws63_team_network_app.c", app_source, "physical first hop"),
    ("ws63_team_network_app.c", app_source, "team_leader_relay_swap_tick"),
    ("ws63_team_network_app.c", app_source, "SLE_TEAM_RELAY_SWAP_STABLE_S"),
    ("ws63_team_network_app.c", app_source, "team_ws2812_cli_set_rgb"),
    ("ws63_team_network_app.c", app_source, "#define SLE_TEAM_WS2812_IDLE_R 16U"),
    ("ws63_team_network_app.c", app_source, "#define SLE_TEAM_WS2812_LEADER_G 16U"),
    ("ws63_team_network_app.c", app_source,
     "team_ws2812_cli_set_rgb(line[4] == 'r' ? 16U : (line[4] == 'w' ? 10U : 0U)"),
    ("ws63_team_network_app.c", app_source,
     'team_cli_match2(line, "buzz beep", "buzz test")'),
    ("ws63_team_network_app.c", app_source, 'team_cli_match2(line, "led tx", "led rx")'),
    ("ws63_team_network_app.c", app_source,
     'team_cli_match2(line, "led active_low", "led active_high")'),
    ("ws63_team_network_app.c", app_source, 'team_cli_match2(line, "led on", "led off")'),
    ("ws63_team_network_app.c", app_source, "static const team_cli_handler_t cli_handlers[]"),
    ("ws63_team_network_app.c", app_source, "team_cli_match2"),
    ("ws63_team_network_app.c", app_source,
     "return (uint8_t)(strcmp(line, first) == 0 || strcmp(line, second) == 0);"),
    ("ws63_team_network_app.c", app_source,
     "team_http_query_number(path, key, (int64_t)min_value, (int64_t)max_value, 255UL, 0UL, &value) != 0"),
    ("ws63_team_network_app.c", app_source, "team_serial_cfg_cli_done"),
    ("ws63_team_network_app.c", app_source, "team_cfg_apply_role"),
    ("ws63_team_network_app.c", app_source,
     "g_team_node.cfg.role == SLE_TEAM_ROLE_MEMBER && g_team_node.cfg.leader_id == leader_id"),
    ("ws63_team_network_app.c", app_source, "team_serial_cfg_cli_save_leader"),
    ("ws63_team_network_app.c", app_source,
     "((on != 0U) ^ (g_team_rt.led_active_low != 0U)) ? GPIO_LEVEL_HIGH : GPIO_LEVEL_LOW"),
    ("ws63_team_network_app.c", app_source, "team_gpio_config_output_level"),
    ("ws63_team_network_app.c", app_source, "memcpy(&route_update, app_packet->body, sizeof(route_update))"),
    ("ws63_team_network_app.c", app_source, "memcpy(&route_update, app_packet.body, sizeof(route_update))"),
    ("ws63_team_network_app.c", app_source, "SLE_TEAM_MAIN_LOOP_SLEEP_MS"),
    ("ws63_team_network_app.c", app_source, "#define SLE_TEAM_DISPLAY_TASK_STACK_SIZE 0x1800"),
    ("ws63_st7789_display.c", app_source_path.with_name("ws63_st7789_display.c").read_text(errors="replace"),
     "ST7789_LVGL_HANDLER_MIN_INTERVAL_MS"),
    ("ws63_st7789_display.c", app_source_path.with_name("ws63_st7789_display.c").read_text(errors="replace"),
     "CONFIG_SLE_TEAM_ST7789_CS_ALWAYS_LOW"),
]:
    if item not in source_text:
        raise SystemExit(f"post-build guard failed: source {source_name} missing {item}")

print("post-build guard passed: team-network app, ST7789/LVGL display, version strings, route-copy guards, and direct-cap source guards are present")
'''


def main(argv: list[str] | None = None) -> int:
    root = repo_root()
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--host", default=os.environ.get("UBUNTU_HOST", "192.168.6.5"))
    parser.add_argument("--port", type=int, default=int(os.environ.get("UBUNTU_PORT", "22")))
    parser.add_argument("--user", default=os.environ.get("UBUNTU_USER", "owen"))
    parser.add_argument("--password", default=os.environ.get("UBUNTU_PASS", ""))
    parser.add_argument("--sdk", default=os.environ.get("UBUNTU_SDK", "/home/owen/workspace/bearpi-pico_h3863"))
    parser.add_argument("--jobs", default=os.environ.get("BUILD_JOBS", "4"))
    parser.add_argument("--self-id", default="1")
    parser.add_argument("--output", default=str(root / LOCAL_OUT_REL))
    args = parser.parse_args(argv)

    sdk = args.sdk.rstrip("/")
    remote_proto = posixpath.join(sdk, REMOTE_PROTO_REL)
    remote_app = posixpath.join(sdk, REMOTE_APP_REL)
    remote_pkg = posixpath.join(sdk, REMOTE_PKG_REL)
    config_path = posixpath.join(sdk, CONFIG_REL)
    local_output = Path(args.output)
    archive_output = reserve_unique_path(versioned_output_path(local_output, VERSION))

    log("WS63 Ubuntu Paramiko build")
    log(f"profile:    {VERSION} unified runtime role (relay swap hysteresis)")
    log(f"host:       {args.user}@{args.host}:{args.port}")
    log(f"sdk:        {sdk}")
    log(f"archive:    {archive_output}")
    log(f"latest:     {local_output}")

    with tempfile.TemporaryDirectory(prefix="ws63_remote_build_") as tmp_name:
        tmp = Path(tmp_name)
        include_tgz = tmp / "include.tgz"
        src_tgz = tmp / "src.tgz"
        app_tgz = tmp / "sle_team_network.tgz"
        log("[local] packing source archives")
        make_archive(root / "include", "include", include_tgz)
        make_archive(root / "src", "src", src_tgz)
        make_archive(root / "xc" / "ws63_team_network", "sle_team_network", app_tgz)

        client = paramiko.SSHClient()
        client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
        client.connect(
            args.host,
            port=args.port,
            username=args.user,
            password=args.password or None,
            timeout=20,
            banner_timeout=20,
            auth_timeout=20,
        )
        try:
            sftp = client.open_sftp()
            remote_tmp = f"/tmp/ws63_remote_build_{os.getpid()}_{int(time.time())}"
            run_remote(
                client,
                f"set -e; test -f {quote(config_path)}; mkdir -p {quote(remote_tmp)} {quote(remote_proto)} {quote(posixpath.dirname(remote_app))}",
                "preflight",
            )
            upload_file(sftp, include_tgz, posixpath.join(remote_tmp, "include.tgz"))
            upload_file(sftp, src_tgz, posixpath.join(remote_tmp, "src.tgz"))
            upload_file(sftp, app_tgz, posixpath.join(remote_tmp, "sle_team_network.tgz"))

            run_remote(
                client,
                "set -e; "
                f"rm -rf {quote(posixpath.join(remote_proto, 'include'))} "
                f"{quote(posixpath.join(remote_proto, 'src'))} {quote(remote_app)}; "
                f"mkdir -p {quote(remote_proto)} {quote(posixpath.dirname(remote_app))}; "
                f"tar -xzf {quote(posixpath.join(remote_tmp, 'include.tgz'))} -C {quote(remote_proto)}; "
                f"tar -xzf {quote(posixpath.join(remote_tmp, 'src.tgz'))} -C {quote(remote_proto)}; "
                f"tar -xzf {quote(posixpath.join(remote_tmp, 'sle_team_network.tgz'))} -C {quote(posixpath.dirname(remote_app))}",
                "sync source",
            )

            lvgl_patch = posixpath.join(remote_app, "third_party/lvgl-patches/lv8.3.11-ws63-c89-rect.patch")
            run_remote(
                client,
                "set -e; "
                f"LVGL_PATCH={quote(lvgl_patch)}; REMOTE_APP={quote(remote_app)}; "
                'if [ -f "$LVGL_PATCH" ]; then '
                'cd "$REMOTE_APP/third_party/lvgl"; '
                'if grep -q "lv_area_t center_coords;" src/draw/sw/lv_draw_sw_rect.c && '
                'grep -q "bool mask_any_center = false;" src/draw/sw/lv_draw_sw_rect.c; then '
                'echo "LVGL patch already present in source"; '
                'elif git apply --unidiff-zero --check "$LVGL_PATCH"; then '
                'git apply --unidiff-zero "$LVGL_PATCH"; '
                'elif git apply --unidiff-zero --reverse --check "$LVGL_PATCH"; then '
                'echo "LVGL patch already applied"; '
                'else echo "LVGL patch check failed: $LVGL_PATCH" >&2; exit 1; fi; fi',
                "apply lvgl patch",
            )

            config_py = posixpath.join(remote_tmp, "configure.py")
            guard_py = posixpath.join(remote_tmp, "post_build_guard.py")
            with sftp.file(config_py, "w") as f:
                f.write(build_config_script())
            with sftp.file(guard_py, "w") as f:
                f.write(post_build_guard_script())

            run_remote(client, f"python3 {quote(config_py)} {quote(config_path)} {quote(args.self_id)}", "configure kconfig")
            run_remote(client, f"cd {quote(sdk)} && python3 build.py -c ws63-liteos-app -j{quote(str(args.jobs))}", "build firmware", timeout=3600)
            run_remote(client, f"python3 {quote(guard_py)} {quote(sdk)}", "post-build guard")

            download_file(sftp, remote_pkg, archive_output)
            pkg_bytes = archive_output.read_bytes()
            if VERSION.encode("ascii") not in pkg_bytes:
                raise RuntimeError(f"downloaded package does not contain {VERSION}: {archive_output}")
            if archive_output != local_output:
                shutil.copy2(archive_output, local_output)
                log(f"[latest] updated {local_output}")
            log(f"[done] package size={archive_output.stat().st_size} contains_{VERSION}=True archive={archive_output}")
            run_remote(client, f"rm -rf {quote(remote_tmp)}", "cleanup")
        finally:
            client.close()

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
