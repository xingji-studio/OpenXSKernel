#include <input_sink.h>
#include <syscall/pxapi.h>
#include <dlinker.h>
#include <krlibc.h>

// 静态表：BSS 零初始化即空表、无焦点。
static input_sink_t *g_sinks[INPUT_SINK_MAX];
static uint32_t g_focus_id = 0;   // 0 = 无焦点

// 输入流拦截器（内核态 IME manager）。NULL = 无拦截器，走快路径。
static input_interceptor_fn g_interceptor = NULL;

void input_route_set_interceptor(input_interceptor_fn fn)
{
    g_interceptor = fn;
}

input_interceptor_fn input_route_interceptor(void)
{
    return g_interceptor;
}

int input_sink_register(input_sink_t *sink)
{
    if (sink == NULL) return -1;
    if (sink->id != 0) return (int)sink->id;   // 已注册
    for (uint32_t i = 0; i < INPUT_SINK_MAX; i++) {
        if (g_sinks[i] == NULL) {
            g_sinks[i] = sink;
            sink->id = i + 1;                   // id 从 1 起，0 保留为"未注册"
            return (int)sink->id;
        }
    }
    return -1;
}

input_sink_t *input_sink_lookup(uint32_t id)
{
    if (id == 0 || id > INPUT_SINK_MAX) return NULL;
    return g_sinks[id - 1];
}

void input_sink_set_focus(uint32_t id)
{
    g_focus_id = id;
}

input_sink_t *input_sink_focus(void)
{
    return input_sink_lookup(g_focus_id);
}

// 导出输入原语，供 OpenXJ380 IME 兼容 kmod 注册拦截器与焦点 sink。
EXPORT_SYMBOL(input_route_set_interceptor);
EXPORT_SYMBOL(input_route_interceptor);
EXPORT_SYMBOL(input_sink_register);
EXPORT_SYMBOL(input_sink_lookup);
EXPORT_SYMBOL(input_sink_set_focus);
EXPORT_SYMBOL(input_sink_focus);
