# Mac 上快速安装 x86_64 Debian VM

这个目录保存本次 WS63 编译环境用到的 Debian netboot 启动文件和自动安装配置，目的是让 Apple Silicon Mac 用户更快复现 `WS63 LiteOS` 编译环境。

## 文件说明

- `boot/linux`：Debian netboot installer kernel
- `boot/initrd.gz`：Debian netboot installer initrd
- `preseed.cfg`：自动安装配置

这不是完整虚拟机磁盘镜像，只是安装入口和预置配置。

## 安全提醒

`preseed.cfg` 为了本地快速复现，使用了弱密码：

```text
root/root
codex/codex
```

只建议用于本机开发 VM，并且 QEMU 端口转发应绑定到 `127.0.0.1`。

如果这台 VM 会暴露到局域网或公网，请安装完成后立刻改密码：

```sh
passwd
sudo passwd root
```

或者自行修改 `preseed.cfg` 中的密码字段后再安装。

## 推荐安装流程

先创建 Debian 磁盘：

```sh
mkdir -p "$HOME/Downloads/debian-mini"
qemu-img create -f qcow2 "$HOME/Downloads/debian-mini/debian-x86_64.qcow2" 40G
```

启动安装器：

```sh
qemu-system-x86_64 \
  -m 3072 \
  -smp 2 \
  -cpu qemu64 \
  -kernel vm-preseed/boot/linux \
  -initrd vm-preseed/boot/initrd.gz \
  -drive "file=$HOME/Downloads/debian-mini/debian-x86_64.qcow2,if=virtio,format=qcow2" \
  -netdev user,id=n1,hostfwd=tcp:127.0.0.1:2222-:22 \
  -device e1000,netdev=n1 \
  -append "auto=true priority=critical preseed/url=file:///preseed.cfg console=ttyS0" \
  -display cocoa
```

如果 `file:///preseed.cfg` 没有被安装器读取，可以改用手动挂载、HTTP 提供 preseed，或在图形安装流程里手动完成最小安装。

## 安装后启动 VM

安装完成后从硬盘启动：

```sh
qemu-system-x86_64 \
  -m 3072 \
  -smp 2 \
  -cpu qemu64 \
  -drive "file=$HOME/Downloads/debian-mini/debian-x86_64.qcow2,if=virtio,format=qcow2" \
  -netdev user,id=n1,hostfwd=tcp:127.0.0.1:2222-:22 \
  -device e1000,netdev=n1 \
  -boot order=c \
  -display cocoa
```

SSH 登录：

```sh
ssh -p 2222 codex@127.0.0.1
```

默认密码：

```text
codex
```

## WS63 编译依赖

进入 VM 后安装依赖：

```sh
sudo apt update
sudo apt install -y rsync file build-essential cmake ninja-build python3-pip python3-venv git make
```

同步 SDK 时注意不要带根目录旧 `output`：

```sh
rsync -a --exclude=/output source/ bearpi-pico_h3863_fresh/
```

完整编译和烧录踩坑见：

- [`../xc/bearpi_pico_h3863_blinky/WS63_MAC_FLASH_NOTES.md`](../xc/bearpi_pico_h3863_blinky/WS63_MAC_FLASH_NOTES.md)
