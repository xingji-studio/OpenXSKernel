# OpenXSKernel 开发与维护规范

## 建议环境

Linux 或 WSL，安装 Python 3、Ninja、Clang/LLD、NASM。构建图由 `tools/gen_ninja.py` 生成。

## 标准验证流程

```bash
python3 -m unittest discover -s tests -v   # 宿主机侧测试
python3 tools/gen_ninja.py                 # 重新生成构建图
ninja                                      # 构建内核
```

涉及启动、驱动、文件系统或用户可见行为时，还应配合 OpenXSBoot + LeopardOS 在 QEMU 中启动并
检查串口日志（见 LeopardOS 的 `TESTING.md`）。

## 代码与注释

- 第一方 C/C++ 使用根目录 `.clang-format`，控制在 120 列以内。
- 注释解释约束、所有权、并发或硬件原因，不复述语句本身。
- 调试输出必须有明确作用域，合入前删除一次性日志和个人路径。
- 新公共头文件使用 `#pragma once`，并只包含声明所需的最小依赖。

## 变更边界

- **内核只收原语、不收具体业务**（见根 `AGENTS.md` RED LINES）；发现非原语业务改动先警告。
- **上游原版 bug 不改**；先归因，只修自己改动引入的 bug。
- 修改系统调用时同步检查三处：`sys.cpp` 实现、`syscall.h` 声明、`syscall.cpp` 分发。
- 修改 VFS/文件句柄时检查 open、dup、close、进程复制和错误回滚路径。
- 修改启动或内存布局时同时审查 HHDM、链接脚本和页表初始化（与 OpenXSBoot 交接）。
- 修改构建图时同步 `tools/gen_ninja.py`；`build.ninja` 是生成文件，不手改。

## 提交前清单

1. `git diff --check` 无空白错误。
2. `python3 tools/gen_ninja.py` 重新生成后 `ninja` 构建通过。
3. 未改写 vendored 第三方库（`liballoc-x86_64.a`、FATFS 上游代码）。
4. 未内嵌具体业务/发行版路径/文件名。
5. 构建或运行受环境限制时，在审查说明中明确列出未执行项及原因。
