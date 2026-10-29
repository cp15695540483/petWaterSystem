/**
 * @file PetWaterSystem.ino
 * @brief 基于 FreeRTOS 的智能宠物饮水系统 - 主程序
 * @note 开发环境：Arduino IDE，开发板选择 ESP32 Dev Module 或 ESP32-WROOM-DA Module
 *       Blinker 运行于 loop，需增大 loop 任务栈以避免 Double exception
 */

#include "config.h"
#include "pump_control.h"
#include "sensor_task.h"
#include "blinker_comm.h"
#include "alarm_task.h"
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

/* loop 任务栈增至 16KB，供 Blinker/WiFi/MQTT 使用，避免栈溢出导致 Guru Meditation */
#if defined(ESP_LOOP_TASK_STACK_SIZE)
ESP_LOOP_TASK_STACK_SIZE(16384);
#elif defined(SET_LOOP_TASK_STACK_SIZE)
SET_LOOP_TASK_STACK_SIZE(16 * 1024);
#endif

#define BTN_DEBOUNCE_MS  80
#define BLINKER_REPORT_INTERVAL_MS  500
#define DEBUG_STATUS_INTERVAL_MS   10000   /* 每 10 秒打印一次运行状态 */

static uint32_t s_lastBlinkerReport = 0;
static uint32_t s_lastDebugStatus = 0;
static bool s_firstLoop = true;
static uint32_t s_btnEmergencyLast = 0;
static uint32_t s_btnResetLast = 0;

void setup() {
#if defined(DEBUG_SERIAL)
    Serial.begin(115200);
    delay(500);
    DBG_PRINTLN(F("[系统] PetWaterSystem 启动..."));
#endif

    pinMode(PIN_BTN_EMERGENCY, INPUT_PULLUP);
    pinMode(PIN_BTN_RESET, INPUT_PULLUP);

    pump_init();
#if defined(DEBUG_SERIAL)
    DBG_PRINTLN(F("[系统] 水泵初始化完成"));
#endif

    xTaskCreate(pump_task, "pump", TASK_PUMP_STACK, NULL, TASK_PUMP_PRIO, NULL);
    xTaskCreate(sensor_task, "sensor", TASK_SENSOR_STACK, NULL, TASK_SENSOR_PRIO, NULL);
    xTaskCreate(alarm_task, "alarm", TASK_ALARM_STACK, NULL, TASK_ALARM_PRIO, NULL);
#if defined(DEBUG_SERIAL)
    DBG_PRINTLN(F("[系统] 任务创建完成: pump, sensor, alarm"));
#endif

    /* 延时使 WiFi 子系统就绪后再初始化 Blinker */
#if defined(DEBUG_SERIAL)
    DBG_PRINTLN(F("[系统] 延时 2 秒，等待 WiFi 子系统就绪..."));
#endif
    delay(2000);

#if defined(DEBUG_SERIAL)
    DBG_PRINTLN(F("[Blinker] 正在连接 WiFi 与 Blinker 云（可能需 5~15 秒）..."));
#endif
    blinker_init();
#if defined(DEBUG_SERIAL)
    DBG_PRINTLN(F("[Blinker] 连接成功，设备已上线"));
    DBG_PRINTLN(F("[系统] 进入主循环"));
#endif
}

void loop() {
    uint32_t now = millis();

#if defined(DEBUG_SERIAL)
    if (s_firstLoop) {
        s_firstLoop = false;
        DBG_PRINTLN(F("[loop] 主循环已启动"));
    }
#endif

    /* Blinker 驱动：需频繁调用以维持 MQTT 心跳与响应 */
    blinker_run();

    /* 处理待推送告警（由 alarm_task 请求，在此执行，避免 alarm_task 栈溢出） */
    blinker_processNotify();

    /* 周期上报水位、TDS 到 Blinker APP */
    if ((now - s_lastBlinkerReport) >= BLINKER_REPORT_INTERVAL_MS) {
        s_lastBlinkerReport = now;
        blinker_reportData();
    }

#if defined(DEBUG_SERIAL)
    /* 每 10 秒打印运行状态 */
    if ((now - s_lastDebugStatus) >= DEBUG_STATUS_INTERVAL_MS) {
        s_lastDebugStatus = now;
        DBG_PRINTF("[状态] 水位=%.1f%% TDS=%d ppm 水泵=%s\n",
            sensor_getWaterLevel(), sensor_getTDS(),
            pump_isRunning() ? "运行" : "停止");
    }
#endif

    /* 按键扫描 */
    if (digitalRead(PIN_BTN_EMERGENCY) == LOW) {
        if ((now - s_btnEmergencyLast) >= BTN_DEBOUNCE_MS) {
            s_btnEmergencyLast = now;
            pump_requestManualFill(0);
        }
    }

    if (digitalRead(PIN_BTN_RESET) == LOW) {
        if ((now - s_btnResetLast) >= BTN_DEBOUNCE_MS) {
            s_btnResetLast = now;
            pump_clearFault();
        }
    }

    vTaskDelay(pdMS_TO_TICKS(20));
}
