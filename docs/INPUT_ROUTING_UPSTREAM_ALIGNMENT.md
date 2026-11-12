# OpenXJ380 内核态输入/IME 实现 — 上游对齐结论

> 依据：OpenXJ380 仓库（HEAD a1fcc2f）实际代码逐文件核对。
> 目的：把之前「从零重构」的方向纠正为「参考/移植 OpenXJ380 已有内核态实现」。

## 一句话结论

OpenXJ380 的输入法与输入路由**已经是完整的内核态子系统**，不是零散钩子。
官方说的「内核态全部」指的就是这套东西：状态机、焦点、事件队列、投递、
过滤、配置 registry、用户数据存储，全部在 `kernel/` 里；「引擎」和「管理器」
只是内核按需拉起、通过 syscall 与之对话的**用户态进程**。

## 一、完整文件清单（OpenXJ380 已有）

| 文件 | 行数 | 职责 |
|---|---|---|
| `include/input_method.h` | 57 | IME 内核接口声明（focus/delivery/mode/filter/syscall/launch） |
| `include/xime.h` | 232 | IME ABI：V1+V2 线格式，含 static_assert 锁布局 |
| `kernel/input_method.cpp` | 812 | **核心**：状态机 + 焦点 + 事件队列 + 投递 + syscall 面 |
| `kernel/ime_config.cpp` | 550 | 配置 registry 加载/保存 |
| `kernel/ime_user_store.cpp` | 342 | 用户数据（词库等）存储 |
| `kernel/ime_internal.h` | 20 | 内部声明 |
| `kernel/syscall/message.cpp` | — | `do_message` → `ime_filter_message` → `do_message_unfiltered` |
| `kernel/user/x3tp.cpp` | — | 终端 xtttp 原语（input/output 整行缓冲） |

OpenXSKernel 目前**以上文件一个都没有**（已核实 `ls` 全部不存在）。

## 二、三个开放问题的官方答案（来自实际代码）

### Q1 拦截器形态 → 固定内核函数，不是函数指针注册

```c
// kernel/syscall/message.cpp:199
void do_message(msg_type, hData, lData, WinMpf, ftask)
{
    if (ime_filter_message(msg_type, lData, (uint64_t)WinMpf, ftask)) return; // 拦截=已消费
    do_message_unfiltered(...);
}
```

`ime_filter_message` 是 `kernel/input_method.cpp:302` 的**固定内核函数**，
不是可注册回调。判断链：类型过滤 → 焦点/会话/密码上下文匹配 → 修饰键/Shift 单点
→ 英文模式短路 → 入事件队列（满则 fail-open）。返回 `true` = 已吞。

### Q2 焦点表示 → 内核存完整身份，不是 sink id

```c
struct ImeFocus {
    uint64_t pid, tid, window, control, callback;
    uint32_t flags;
    int32_t  x, y;
};
```

内核**直接存 pid/tid/window/control/callback**（注释明说「Core owns the session;
graphics owns and validates window lifetimes」）。我之前「内核只认 sink id、window→sink
归 win32k」的简化**与上游相反**——上游内核就是存 window/control 全套身份。

### Q3 do_message 语义 → 键盘路径就是 do_message，无独立 input_route_dispatch

```
键盘/图形 worker → do_message → ime_filter_message(IME 内核态) → do_message_unfiltered
                                                                    ├─ ime_text_sink（文本汇）
                                                                    ├─ getch 环形缓冲（MSG_CHAR）
                                                                    └─ WinMpf → message pipe
```

上游**没有** `input_route_dispatch` 这种独立路由函数。我 Phase 2 拆出来的
`input_route_dispatch` 是与上游不一致的自造结构。

## 三、我 Phase 1-3 + SPSC 与上游的逐项偏差

| 维度 | OpenXJ380 上游 | 我在 OpenXSKernel 做的 | 偏差 |
|---|---|---|---|
| 拦截 | `ime_filter_message` 固定函数 | `input_route_set_interceptor` 函数指针注册 | 形态不符 |
| 焦点 | `ImeFocus{pid,tid,window,control,callback,flags,x,y}` | `input_sink` 只存 id | 数据不符 |
| getch 环 | 挂在 `xtttp_dtt` 终端上，全局 `getch_lock` spinlock | 独立 `input_sink_t`，SPSC 无锁 | 位置+机制均不符 |
| 路由 | `do_message` 单入口 | 拆成 `input_route_dispatch`/`do_message` | 自造分叉 |
| syscall | `XAPI_IME_*` 完整面（REGISTER/ENUM/ATTACH/GET_EVENT/REPLY/UPDATE_UI/SET_MODE/USER_DATA） | 无 | 缺失整套 |

## 四、三个角色（澄清「管理器在内核态」）

1. **内核态 IME 核心**（`kernel/input_method.cpp`）：状态机/焦点/队列/过滤/registry —— 这才是「内核态全部」的实体。
2. **管理器进程**（`ctrlmenu.elf`，`ime_role=2`）：用户态，经 `ime_syscall` 做 REGISTER/SET_ACTIVE/UNREGISTER/SET_MODE/ENUM + 用户数据 INFO/CLEAR/SET_ENABLED。
3. **引擎进程**（`inmeth_pinyin.elf`，`ime_role=1`）：用户态，ATTACH/GET_EVENT/REPLY_EVENT/UPDATE_UI/SET_MODE。

「管理器在内核态」的准确含义是：**IME 的路由/状态/过滤逻辑在内核态**，管理器与引擎只是挂在这套内核状态机上的用户态进程。

## 五、建议方向（待你拍板）

**A. 移植上游**（推荐）：把 OpenXJ380 的 `input_method.*`、`xime.h`、`ime_config.*`、
`ime_user_store.*`、`ime_internal.h` 整套移植进 OpenXSKernel，按 OpenXSKernel 的
构建/风格适配；把我 Phase 1-3 + SPSC 的 4 个 commit 按需 revert 或降级为终端原语。

**B. 仅对齐不移植**：保留我 input_sink 作为底层原语，但把拦截器改回 `ime_filter_message`
式固定函数、焦点改回 `ImeFocus` 全套身份、去掉 `input_route_dispatch` 自造分叉。

**C. 先暂停，等你再跟官方确认一次**（比如「管理器/引擎到底是不是用户态进程」这句）。

在拿到你方向前，我不再往 OpenXSKernel 提交新代码。
