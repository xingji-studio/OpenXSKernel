# 安全加固现状（纯内核）

> 本文档曾记录 OpenXJ380 全系统（含 GUI、用户注册表 usereg.dat、可加载模块签名、TLS/chat、
> busybox）的安全加固。纯内核抽取后，用户态相关项已移到 LeopardOS / 组件仓库；本文件只保留
> **内核层仍适用**的加固项。

## 内核层已修复 / 已加固

- 用户 ELF 装载：校验 ELF/program-header/file 范围与地址空间边界；拒绝截断头、溢出段表、
  内核/null 页段地址、超限映射、入口点落在可执行段之外。
- GPT/MBR 解析：限制全局与单盘分区数，检查乘法/LBA 算术、磁盘边界与分配结果。
- 系统调用边界：用户指针先经当前进程页表复制，绝不直接对用户指针调用 `strlen/strncpy/memcpy`。
- VFS 符号链接与别名：递归解析限制 40 层。
- 全路径构造：动态分配结果，不写死 256 字节缓冲。
- 内核随机：`getrandom` / `/dev/urandom` 共享带锁持久 ChaCha20 发生器，优先 `RDSEED`/`RDRAND`。

## 已移出内核 / 不适用

- `usereg.dat` 凭据存储与 OOBE 认证流 → 用户态（LeopardOS 组件）。
- 可加载模块签名/信任策略 → 纯内核已无 kmod（无 `kmod/` 目录）。
- TLS/chat、busybox/libvterm → 用户态，不在本仓库。
- GUI（窗口/绘制/按钮/滚动条/输入框）→ 已移除，对应 XAPI syscall 保留号但 stub 为 `-ENOSYS`。

## 验证

- `python3 tools/gen_ninja.py && ninja` 内核构建通过。
- 宿主机侧单元测试 `python3 -m unittest discover -s tests -v`。
- 运行时验证配合 OpenXSBoot + LeopardOS 在 QEMU 启动，检查串口日志。
