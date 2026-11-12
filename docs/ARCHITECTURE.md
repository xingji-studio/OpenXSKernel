# OpenXSKernel 架构与目录边界

本文说明纯内核仓库的稳定分层、依赖方向和代码归属。它不描述内部 ABI 的全部细节；涉及启动
顺序、中断帧、页表或系统调用 ABI 的改动，仍应结合相应目录中的 `AGENTS.md` 审查。

## 启动与运行路径

```text
UEFI firmware
    -> OpenXSBoot (BOOTX64.efi，兄弟仓库)
    -> kernel/KernelMain (out/kernel.krl)
    -> kernel core + built-in driver/
    -> VFS/root filesystem
    -> launch PID1 (BootConfig.init，来自 boot.conf)
    -> user-space ELF applications（用户态，本仓库之外）
```

引导器在 OpenXSBoot 仓库；本仓库只负责固件交接后的一切：CPU/内存/设备/VFS/驱动/SMP/系统
调用/进程，以及最后按 `BootConfig.init` 启动 PID1。live 环境下（`BOOT_FLAG_LIVE_ROOT`），
`kernel/installer_mode.cpp` 把 XJPAK10 live root 解包进 tmpfs 当根文件系统。

## 目录职责

| 目录 | 职责 | 依赖边界 |
|---|---|---|
| `driver/` | 必须随内核启动的内建驱动和文件系统 | 链接进内核；不包含可加载模块（已无 kmod） |
| `kernel/` | CPU、内存、任务、中断、系统调用和核心启动 | 可依赖共享 `include/`，不能依赖用户态实现 |
| `font/` | 内核侧字体资源（hankaku.bin） | 构建期 objcopy 进镜像 |
| `include/` | 内核、驱动共享的 ABI 头文件 | 头文件小而明确，避免引入实现细节 |
| `tools/` | `gen_ninja.py` 构建图生成器 | 不进入目标系统运行时 |
| `tests/` | 宿主机侧 Python 测试 | 不进入目标系统运行时 |
| `docs/` | 架构/构建/开发说明 | — |

已不存在（OpenXJ380 时代）：`boot/`、`kmod/`、`third_party/`、顶层 `user/`、
`Bf/`、`resources/`、`graphics/`、`lib/`。

## 依赖规则

1. 内核核心不得反向依赖用户态应用。
2. 用户指针必须在系统调用边界经过校验或复制，VFS/驱动不得直接信任用户缓冲区。
3. 生成文件（`build.ninja`、`kernel/build_config.h`、`.clangd`）不手工维护。
4. 内核只收原语、不收具体业务（详见仓库根 `AGENTS.md` RED LINES）。
5. 上游原版代码的 bug 不改，只修自己改动引入的 bug。

## 高风险契约

- 页表 / HHDM 高半区布局必须与 OpenXSBoot 交接一致。
- `kernel/intr/` 的保存寄存器布局是汇编、调度器和系统调用共同使用的 ABI。
- `include/syscall/syscall.h` 的 syscall 号与数据结构必须和 `sys.cpp` 实现、
  `syscall.cpp` 分发三处同步。
- VFS 节点、文件句柄具有跨子系统生命周期，修改所有权前必须审计 close/dup/复制路径。
- `liballoc-x86_64.a` 在链接期通过 `--wrap` 包裹，其 IRQ 安全约束见
  `SCHEDULER_AND_LOCKING.md`。
