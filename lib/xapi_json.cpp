#include <krlibc.h>
#include <mm/alloc/alloc.h>
#include <xapi_json.h>
#include <dlinker.h>

#include "xapi_json_impl.inc"

// 导出 JSON 解析原语，供 ime-kmod（ime_config）在加载时解析。
EXPORT_SYMBOL(xapi_parseJson);
EXPORT_SYMBOL(xapi_freeJsonValue);
EXPORT_SYMBOL(xapi_freeJsonDocument);
EXPORT_SYMBOL(xapi_jsonObjectGet);
EXPORT_SYMBOL(xapi_jsonArraySize);
EXPORT_SYMBOL(xapi_jsonArrayAt);
