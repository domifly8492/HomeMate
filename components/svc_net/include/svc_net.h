#pragma once
#include <stdbool.h>
#include "esp_err.h"

/* 启动网络服务：连接 Wi-Fi，断线自动重连；连上后自动对时。
 * 要在 app_events_init() 之后调用。它通过事件通知别人：
 *   EVT_WIFI_CONNECTED / EVT_WIFI_LOST / EVT_TIME_SYNCED */
esp_err_t svc_net_start(void);

bool svc_net_is_connected(void);
bool svc_net_time_valid(void);          /* 对时成功过吗 */

/* 信号等级 0~4，没连上返回 -1。rssi_out 不需要时传 NULL */
int  svc_net_wifi_level(int *rssi_out);