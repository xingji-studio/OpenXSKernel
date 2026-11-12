# 输入流路由原语 — 架构设计

本文定义 OpenXSKernel 对输入路径的架构级重构目标：把「键盘 → 消息 → 终端」这条被
IME/GUI 特化的链路，重构为 focus-agnostic 的「源 / 拦截 / 路由 / 汇」四件套原语。

> 状态：设计文档（未实施）。OpenXJ380 移植是后续决策，当前只在 OpenXSKernel 上做。

## 目标与非目标

**目标**

- 输入流是 focus-agnostic 的原语：物理键盘、虚拟键盘、IME 引擎、GUI 文本框都只是这条流上的
  生产者/消费者。
- 内核只负责「注册 sink + 设焦点 + 注入字节 + 读字节」，不关心字节是拼音还是软键盘还是图形
  输入框。

**非目标（红线）**

- 不引入 IME 业务：拼音合成、候选分页、词库、session/UI（xime.h 那套）不进内核。
- 不引入 GUI 业务：窗口管理、GDI、桌面环境不进内核——归未来闭源 win32k 式内核模块。
- 不引入 window/control/callback/坐标 作为内核路由身份；内核只认 sink id。

## 现状链路与冗余

```text
物理键盘 → (闭源 GUI worker) → do_message → ime_filter_message → do_message_unfiltered → getch 环形缓冲
```

冗余点：

1. `ime_filter_message` 每次 mutex + 一串 IME 检查，即使没挂 IME 引擎也全套跑。
2. 焦点匹配用 `ImeFocus` 五字段（pid/tid/window/control/callback），是 GUI 身份，不是路由原语。
3. `do_message_unfiltered` 里终端 getch 路径与 GUI message-pipe 路径共用一个函数分叉。
4. getch 消费端是 busy-poll + scheduler_yield。

## 目标架构

```text
物理键盘 ──┐
虚拟键盘 ──┼→ input_route_dispatch(event) ──┬─ 无拦截器（快路径）→ 焦点 sink
IME 提交 ──┘                                  └─ 有拦截器（慢路径）→ 拦一下 → 改道/吞掉/提交 → 焦点 sink
```

四件套：

| 原语 | 职责 | 落点 |
|---|---|---|
| 源 source | 往输入流注入事件/字节 | 泛化 SXAH_WRITE_INPUT_BUFFER |
| 拦截 interceptor | 可注册的流上钩子，放行/吞掉/替换/改道 | 泛化 ime_filter_message |
| 路由 router | 维护焦点 sink，投递事件 | input_route_dispatch + 焦点 sink 表 |
| 汇 sink | 消费字节 | 每终端 SPSC 环形缓冲 + getch 读端 |

## 关键决策

### 1. 快/慢路径分叉

`input_route_dispatch` 先查「有无注册拦截器」：

- 无拦截器 → 直接投递焦点 sink，不碰 mutex、不碰 IME 检查（快路径）。
- 有拦截器 → detour 到拦截器，拦完再投递（慢路径）。

拦截器位置（内核态模块 vs 用户态进程）因此只影响「IME 活动时」的延迟，不卡日常输入快路径。

### 2. 焦点 sink 表（O(1) 路由）

内核维护「焦点 sink id → sink 结构」表，`input_route_dispatch` 一次索引完成路由，
替代 `ImeFocus` 五字段匹配。window→sink 映射归 win32k 模块。

### 3. do_message 兼容别名

闭源 GUI worker 仍调 `do_message(type,hData,lData,WinMpf,ftask)`。保留该签名，内部走
`input_route_dispatch` 快路径。不要求对方改调用方式。

### 4. 汇传输机制（优先级）

1. waitqueue 唤醒（消费端 busy-poll 改睡眠等待，收益最大）
2. 拆全局锁 → 每终端独立 sink
3. 无锁 SPSC（head/tail 分离，最后做；需先补 barrier 原语）

## 待官方口径（实施前确认）

1. 拦截器注册形态：内核态可加载模块接口，还是用户态进程 + syscall 注册？
2. 焦点 sink 表所有权：内核存裸 id，还是 win32k 模块设焦点？
3. 键盘 source 直连：OpenXSKernel 无 GUI，键盘中断直接进 input_route_dispatch；
   闭源 GUI 场景由 worker 调 do_message。

## 实施阶段

- Phase 1：路由入口 input_route_dispatch + do_message 兼容别名 + 快路径
- Phase 2：焦点 sink 表（O(1) 路由）
- Phase 3：拦截钩子（快慢路径分叉）
- Phase 4：源注入原语泛化（虚拟键盘 / IME 提交）
- 每阶段编译验证 + 提交；全部结束后再评估是否移植 OpenXJ380。

## ABI 兼容面清单（硬约束，不可破坏）

闭源 win32k 式内核模块是独立模块，只能访问导出的符号与 syscall 号。以下四项是它依赖的
稳定面，本次重构一律不得破坏：

1. `do_message(msg_type, hData, lData, WinMpf, ftask)` 入口签名 —— 保留为兼容别名，内部走
   `input_route_dispatch` 快路径。
2. SXAH syscall 号（`SXAH_WRITE_INPUT_BUFFER` / `READ_OUTPUT_BUFFER` / `MARK_IS_TERMINAL` /
   `CHECK_INPUT_BUFFER` / `UNLOCK_OUTPUT_LOCK` 等）—— 已与 OpenXJ380 同步，只增不改。
3. `ime_set_focus` 入口 —— 作为兼容垫片保留；内部把 window/control 翻译成 sink id 后丢弃，
   翻译表将来归 win32k 模块接管。
4. message pipe + `MessageInfoFormat` 格式 —— GUI 窗口消息投递路径，保持字节布局不变。

内部 sink 结构、焦点表、getch 缓冲布局均为内核私有实现，闭源模块不可见，可自由重构。

## 实施阶段（收敛后，数据先行）

- Phase 1：独立 sink 结构 + 焦点 sink 表（数据基础）
- Phase 2：路由入口 input_route_dispatch + do_message 兼容别名，O(1) 投递
- Phase 3：拦截钩子（快慢路径分叉）
- Phase 4：源注入原语泛化（虚拟键盘 / IME 提交）
