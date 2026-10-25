# OpenXSKernel

OpenXSKernel 是从 OpenXJ380 抽取出来的**纯内核**（kernel-only）仓库。它只构建一个产物
`out/kernel.krl`，**不包含**引导器、用户态应用、包管理器或第三方 vendored 源码——这些
分别放在兄弟仓库（OpenXSBoot、LeopardOS、各 `leopard-*` / `*-xbps` 组件仓库）。

构建图由 `tools/gen_ninja.py` 生成，只有两个一等构建目标：`build`（→ `out/kernel.krl`）
和 `pack`（→ gzip 压缩的 xbps 包）。

## ⚠️ 内核只收原语，不收具体业务

**这是本仓库最重要的规则。** 内核只提供**机制与原语**，绝不内嵌任何**发行版 / 应用 /
产品特定**的策略：

- ✅ **属于原语**：系统调用本身（open/read/write/fstat/mount/brk/…）、文件系统驱动、
  VFS 节点/别名原语、VFAT 文件名规范化（非法字符处理 + LFN）、内存/任务/中断/驱动子系统。
- ❌ **属于业务（禁止进内核）**：具体的发行版文件名（例如把 xbps 密钥文件名硬编码成 VFS
  别名）、具体的应用路径（shell.elf、installer、…）、GUI 窗口/按钮/滚动条 API、
  用户注册表格式、按进程名做调试追踪（`strstr(name, "xbps")`）。

需要业务策略时，一律放在**用户态**（LeopardOS / 各组件仓库），通过通用内核原语驱动——
绝不在内核里硬编码。

> 给开发者 / Agent：如果你发现自己想往内核里写非原语业务，**停下，先警告再讨论**。

## 特性概览

- 面向 x86_64 的单体内核，内核入口为 `KernelMain`。
- 内建驱动：AHCI/IDE/NVMe、PCI、PS/2 键鼠、串口、HDA 音频。
- 文件系统：VFS 核心 + FATFS（FatFs 移植）+ dev/proc/tmp/pipe/pty 等伪文件系统。
- 内存管理、进程/线程、SMP、中断、系统调用基础设施。
- **Linux ABI 系统调用面**（约 264 个 `SYS_*`）：musl 静态二进制（xbps、busybox 等）
  依赖的 POSIX-ish 接口。
- **XAPI 遗留应用接口**：GUI 子集（窗口/绘制/按钮/滚动条/输入框等）已 stub 成
  `-ENOSYS`（syscall 号保留、实现移除）；非 GUI 的 XAPI（时间/睡眠/电源/进程列表等）保留。
- PID1 解耦：内核从 `BootConfig.init`（boot.conf）读 PID1 路径，不再硬编码 shell 路径。
- live 环境支持：`BOOT_FLAG_LIVE_ROOT` 时把 XJPAK10 live root 解包进 tmpfs 作为根文件系统。

## 目录结构

```text
driver/                 链接进 kernel.krl 的内建驱动
  ahci/ fs/ hda/ ide/ nvme/ pci/ ps2/ serial/
font/                   内核侧字体（hankaku.bin，objcopy 进镜像）
include/                内核与驱动共享的 ABI 头文件
kernel/                 内核核心子系统
  cpu/ intr/ memory/ pctable/ smp/ syscall/ task/ user/ wsod/
tests/                  宿主机侧 Python 测试
tools/                  gen_ninja.py —— 唯一构建图生成器
docs/                   架构/构建/开发说明
linker.ld               内核链接脚本（ENTRY(KernelMain)）
liballoc-x86_64.a       预置第三方分配器（链接期 wrap）
```

**不包含**（有意为之）：`boot/`、`kmod/`、`third_party/`、顶层 `user/`、
`Bf/`、`resources/`、`graphics/` —— 这些是 OpenXJ380 时代的东西，现已拆到别的仓库或删除。

## 构建

```bash
python3 tools/gen_ninja.py   # 重新生成 build.ninja（只构建内核的图）
ninja                        # 构建 → out/kernel.krl
ninja pack                   # 打 xbps 包（gzip 压缩；需 xbps-create 在 PATH）
ninja clean                  # 清理 out/、kernel/build_config.h、.pkg/
```

构建环境变量：`COMPILER_PREFIX`、`CC`、`CPP`、`NASM`、`LD`、`OBJC`、
`OPT_LEVEL`、`DIAGNOSTICS_COLOR`、`BUILTIN_XHCI`。

## 已知决策 / 问题（本次会话）

- **zstd 数据损坏 = 上游 lazy-alloc bug，不在内核修。** zstd 的大窗口解压会踩中上游
  `kernel/memory/` 里的内存损坏 bug（OpenXJ380 与 OpenXSKernel 逐字节一致）。解法定在
  用户态层：xbps 包和 repodata 改用 **gzip** 压缩（各组件 `gen_ninja.py` + LeopardOS
  `build_liveroot.sh` 已完成）。不要在这里"修"内核分配器。
- **冒号文件名（FAT）：** 正确的原语是 VFAT 驱动层文件名规范化（非法字符处理 + LFN），
  参照 Linux `fs/fat/namei_vfat.c` —— 而不是应用层别名注册。OpenXJ380 硬编码 xbps 密钥
  VFS 别名是反面教材。

## 相关仓库

- 引导器：OpenXSBoot（`out/BOOTX64.efi`）
- live/安装介质与发行版：LeopardOS
- 用户态组件与包：各 `leopard-*`、`initctl`、`poweroff`、`mkfs-fat32` 等仓库

## 许可与第三方组件

本项目采用 [Apache License 2.0](LICENSE)。`liballoc-x86_64.a` 为预置第三方库，其许可见
`LICENSES.md` 或对应来源。
