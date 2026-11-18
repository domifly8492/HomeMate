#pragma once
#include <stddef.h>
#include "esp_err.h"

/* 提醒编号：EVT_REMINDER 事件的参数就是它，界面和语音都靠它找内容。
 * 新增提醒：先在这里加一项，再去 svc_rules.c 的 DEFS 表里加一行 */
typedef enum {
    REMIND_KEYS = 0,        /* 每天：带钥匙 */
    REMIND_UMBRELLA,        /* 今天可能下雨：带伞 */
    REMIND_COUNT,
} reminder_id_t;

/* 启动规则任务。要在 svc_net_start() 之后调用（它要用 NVS） */
esp_err_t svc_rules_start(void);

/* 给界面用：弹窗的标题和说明文字 */
const char *svc_rules_title(reminder_id_t id);
void svc_rules_detail(reminder_id_t id, char *buf, size_t n);