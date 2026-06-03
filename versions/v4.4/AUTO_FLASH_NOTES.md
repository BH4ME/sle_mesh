# Auto Flash Notes

Date: 2026-05-31

## Working Setup

- Build host: `192.168.6.5`
- SSH user: `owen`
- Remote SDK: `/home/owen/workspace/bearpi-pico_h3863`
- Local firmware output: `E:\codex_documents\sle\output_from_vm\team_network_v4_unified_runtime_role\ws63-liteos-app_v4_unified_all.fwpkg`
- Flash port: `COM16`
- USB serial adapter observed as: `USB-SERIAL CH340 (COM16)`

## Build Command

Use the remote Ubuntu machine, not the local VM:

```powershell
bash -lc "cd /mnt/e/codex_documents/sle && UBUNTU_HOST=192.168.6.5 UBUNTU_USER=owen UBUNTU_PASS='<set locally, do not commit secrets>' UBUNTU_SDK=/home/owen/workspace/bearpi-pico_h3863 BUILD_JOBS=4 ./scripts/ws63_build_v4_ubuntu.sh unified"
```

If `sshpass` is missing locally, use the Python/Paramiko path that was confirmed during v4.4:

1. SSH to `192.168.6.5` as `owen`.
2. Upload `include/`, `src/`, and `xc/ws63_team_network/` to the SDK.
3. Patch the remote Kconfig values:

```text
CONFIG_SLE_TEAM_ST7789_X_OFFSET=40
CONFIG_SLE_TEAM_ST7789_Y_OFFSET=53
CONFIG_SLE_TEAM_ST7789_WIDTH=240
CONFIG_SLE_TEAM_ST7789_HEIGHT=135
```

4. Run `python3 build.py ws63-liteos-app -j4` in `/home/owen/workspace/bearpi-pico_h3863`.
5. Download `/home/owen/workspace/bearpi-pico_h3863/output/ws63/fwpkg/ws63-liteos-app/ws63-liteos-app_all.fwpkg` to the local firmware output path above.
6. Confirm remote generated header contains the final display values:

```sh
grep -n -E '#define CONFIG_SLE_TEAM_ST7789_(X_OFFSET|Y_OFFSET|WIDTH|HEIGHT)' \
  /home/owen/workspace/bearpi-pico_h3863/output/ws63/acore/ws63-liteos-app/mconfig.h
```

Expected:

```text
#define CONFIG_SLE_TEAM_ST7789_X_OFFSET 40
#define CONFIG_SLE_TEAM_ST7789_Y_OFFSET 53
#define CONFIG_SLE_TEAM_ST7789_WIDTH 240
#define CONFIG_SLE_TEAM_ST7789_HEIGHT 135
```

## Flash Command

```powershell
python E:\codex_documents\sle\automation\ws63\tools\ws63_auto_burn.py `
  -p COM16 `
  -b 115200 `
  --software-reset-only `
  --reset-command reboot `
  --reset-command-fallback reset `
  --reset-command-delay 0.3 `
  --reset-command-retries 2 `
  --reset-command-retry-gap 0.2 `
  E:\codex_documents\sle\output_from_vm\team_network_v4_unified_runtime_role\ws63-liteos-app_v4_unified_all.fwpkg
```

## Successful Flash Evidence

The working flash run printed all transfers at 100 percent, then:

```text
Done. Reseting device...
EXIT 0
```

After reboot, the board printed:

```text
[display] st7789 ready 240x135 off=40,53 sclk=6 mosi=8 cs=7 dc=9 rst=13
[display] soft-spi enabled mode=0 (cpol=0 cpha=0)
```

## Failure Pattern To Remember

If the build output package updates but the boot log still says `135x240 off=52,40`, the remote Kconfig did not actually take effect. Re-check `mconfig.h` on the remote build host before flashing again.
