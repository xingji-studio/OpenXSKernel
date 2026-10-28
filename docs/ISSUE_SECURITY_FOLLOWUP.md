# 安全后续事项（纯内核）

> 原 OpenXJ380 版本的后续事项（`usereg.dat` 明文密码、模块签名、TLS/chat 熵回调、busybox/
> libvterm 升级、GUI 相关 fuzz）大部分已随纯内核抽取移出本仓库。本文件只列**内核层仍开放**的事项。

## 仍开放（内核层）

- 内核随机在无 `RDSEED`/`RDRAND` 的 CPU 上无正式熵就绪保证；混合了时序/地址状态，
  在宣称密码学保证前需要启动期熵就绪模型 + 设备/中断熵。
- ELF 装载、GPT/MBR 解析、符号链接解析需要持续的 ASAN/UBSAN/libFuzzer 覆盖。

## 已移出 / 不适用

- 版本化凭据记录（usereg.dat 迁移）→ 用户态。
- 内核模块信任根/签名 → 纯内核无 kmod。
- TLS/chat 熵回调与对端认证 → 用户态。
- busybox/libvterm 版本清单 → 用户态。
- GUI 相关引导测试 → GUI 已移除。

## 验收标准（内核层）

- 无 `RDSEED`/`RDRAND` 时 `getrandom` 行为有文档并在硬件/回退两条路径上测试。
- ELF/分区/链接解析的 fuzz 目标在 CI 中持续运行，内核在 QEMU 中启动到用户态。
