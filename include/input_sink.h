#pragma once
#include <stdint.h>
#include <cpu/lock.h>

// 输入流 sink：输入流的消费端，与终端身份解耦。
// 虚拟键盘 / IME / GUI 文本框的输入都汇入一个 sink；内核只认 id，不关心消费者是谁。
#define INPUT_SINK_GETCH_CAP 128
#define INPUT_SINK_BUF_CAP   1024
#define INPUT_SINK_MAX       64

typedef struct input_sink {
    uint32_t id;               // 焦点表中的 id（0 = 未注册）
    spin_t   lock;             // 每 sink 独立锁，保护 getch 环形缓冲
    uint32_t getch_head;
    uint32_t getch_count;
    char     getch_bytes[INPUT_SINK_GETCH_CAP];
    bool     wait_for_getch;
    uint32_t input_flags;
    bool     wait_for_input;
    bool     input_lock;
    char     input[INPUT_SINK_BUF_CAP];
} input_sink_t;

// 输入流拦截器（内核态，IME manager 注册）：返回 true = 已消费（吞掉/改道），
// false = 放行。管理器在内核态，故这是函数指针注册，不是 syscall。
typedef bool (*input_interceptor_fn)(uint64_t msg_type, uint64_t lData);
void               input_route_set_interceptor(input_interceptor_fn fn);
input_interceptor_fn input_route_interceptor(void);

// 焦点 sink 表：sink id -> sink。内核只维护 id 与焦点，window->sink 映射归 win32k 模块。
int           input_sink_register(input_sink_t *sink);   // 返回 id，失败 -1
input_sink_t *input_sink_lookup(uint32_t id);
void          input_sink_set_focus(uint32_t id);
input_sink_t *input_sink_focus(void);

// 从 sink 环形缓冲非阻塞读一个键：-1 表示空（并置 wait_for_getch）。
int message_getch_try(input_sink_t *sink);
