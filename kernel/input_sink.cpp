#include <input_sink.h>
#include <krlibc.h>

// 静态表：BSS 零初始化即空表、无焦点。
static input_sink_t *g_sinks[INPUT_SINK_MAX];
static uint32_t g_focus_id = 0;   // 0 = 无焦点

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
