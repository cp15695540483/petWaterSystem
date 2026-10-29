/**
 * @file blinker_comm.cpp
 * @brief Blinker 物联网通信：数据上报、远程控制、告警推送
 * @note 使用前需在 config.h 配置 BLINKER_AUTH、WIFI_SSID、WIFI_PASSWORD
 */

#define BLINKER_WIFI
#include "config.h"
#include "blinker_comm.h"
/* 须在 Blinker.h 之前包含本模块头文件，避免库内宏与 BLINKER_NOTIFY_* 枚举常量名冲突 */
#include <Blinker.h>
#include "sensor_task.h"
#include "pump_control.h"
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

/* Blinker 组件键名（需与 APP 中配置一致） */
#define BLINKER_KEY_WATER   "num-level"   /* 水位数据 */
#define BLINKER_KEY_TDS     "num-tds"     /* TDS 数据 */
#define BLINKER_KEY_PUMP    "btn-pump"    /* 远程补水按钮 */
#define BLINKER_KEY_TIMER   "max-timer"   /* 定时补水（可选） */

static BlinkerNumber BlinkerWater(BLINKER_KEY_WATER);
static BlinkerNumber BlinkerTDS(BLINKER_KEY_TDS);
static BlinkerButton BlinkerPump(BLINKER_KEY_PUMP);

static void button_pump_cb(const String& state) {
    (void)state;
#if defined(DEBUG_SERIAL)
    DBG_PRINTLN(F("[Blinker] 远程补水按钮已触发"));
#endif
    pump_requestManualFill(0);  /* 0 = 自动补到水位达标 */
}

void blinker_init(void) {
#if defined(DEBUG_SERIAL)
    /* Blinker.begin 为阻塞调用，内部会连接 WiFi 和 MQTT */
    DBG_PRINTLN(F("[Blinker] 调用 Blinker.begin，请稍候..."));
#endif
    Blinker.begin(BLINKER_AUTH, WIFI_SSID, WIFI_PASSWORD);
    BlinkerPump.attach(button_pump_cb);
#if defined(DEBUG_SERIAL)
    DBG_PRINTLN(F("[Blinker] Blinker.begin 返回，初始化完成"));
#endif
}

void blinker_reportData(void) {
    float level = sensor_getWaterLevel();
    int tds = sensor_getTDS();
    BlinkerWater.print((float)((int)(level * 10) / 10.0f));  /* 保留 1 位小数 */
    BlinkerTDS.print(tds);
}

void blinker_notify(const char* msg) {
    Blinker.notify(msg);
}

/* 待推送告警（alarm_task 设置，loop 执行，避免在 alarm_task 栈上调用 Blinker.notify 导致崩溃） */
static volatile BlinkerNotifyType s_pendingNotify = BLINKER_NOTIFY_NONE;

void blinker_requestNotify(BlinkerNotifyType type) {
    s_pendingNotify = type;
}

void blinker_processNotify(void) {
    BlinkerNotifyType type = s_pendingNotify;
    if (type == BLINKER_NOTIFY_NONE) return;
    s_pendingNotify = BLINKER_NOTIFY_NONE;

    const char* msg = "";
    switch (type) {
        case BLINKER_NOTIFY_LOW_WATER:   msg = "缺水告警：水位过低，请及时加水";   break;
        case BLINKER_NOTIFY_OVERFLOW:    msg = "溢水告警：水位过高，水泵已停止";   break;
        case BLINKER_NOTIFY_TDS:         msg = "水质告警：TDS 超标，建议更换水源"; break;
        case BLINKER_NOTIFY_DRY_RUN:     msg = "水泵空转保护：请检查进水管";       break;
        default: return;
    }
#if defined(DEBUG_SERIAL)
    DBG_PRINTLN(F("[Blinker] 告警已推送到 APP"));
#endif
    Blinker.notify(msg);
}

/**
 * @brief 驱动 Blinker 通信（需在主循环中频繁调用）
 * @note 原 blinker_task 已移除，改为在 loop 中调用，避免独立任务栈溢出导致 Double exception
 */
void blinker_run(void) {
    Blinker.run();
}
