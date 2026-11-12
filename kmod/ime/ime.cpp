// kmod/ime/ime.cpp — OpenXJ380 IME 兼容模块
//
// 精简内核只提供输入原语（sink/焦点/拦截器/路由）+ syscall 钩子；IME 业务
// （焦点匹配、事件队列、投递、会话、配置、词库）作为可加载 .sys 模块挂进来。
// dlmain 注册拦截器与 syscall 钩子，让 OpenXJ380 的 XAPI_IME_* 对内核透明。

#include <syscall/pxapi.h>
#include <openxj380/syscall.h>

// 拦截器占位：先注册一个放行实现，证明 kmod 能挂进输入流。IME 业务随后填充。
static bool ime_interceptor(uint64_t msg_type, uint64_t lData, uint64_t callback, tcb_t task)
{
    (void)msg_type; (void)lData; (void)callback; (void)task;
    return false;   // 放行
}

// syscall 钩子占位：接管 XAPI_IME_* 号段（7474-7485）。返回 true = 已处理。
static bool ime_syscall_hook(uint64_t number, struct X64_REGS *regs)
{
    (void)number; (void)regs;
    return false;   // 未处理
}

extern "C"
__attribute__((used)) __attribute__((visibility("default"))) int dlmain(void)
{
    input_route_set_interceptor(ime_interceptor);
    OpenXJ380Socket_RegisterSyscallHook(ime_syscall_hook);
    return 0;
}

extern "C"
__attribute__((used)) __attribute__((visibility("default"))) int dlstart(void)
{
    return 0;
}
