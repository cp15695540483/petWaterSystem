/**
 * @file alarm_task.cpp
 * @brief 告警与 LED 状态指示：正常慢闪、异常快闪/常亮，协同 Blinker 告警推送
 */

#include "config.h"
#include "alarm_task.h"
#include "sensor_task.h"
#include "blinker_comm.h"
#include "pump_control.h"
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define LED_ON_MS   200
#define LED_OFF_SLOW_MS  800   /* 正常：慢闪 */
#define LED_OFF_FAST_MS  100   /* 异常：快闪 */
#define NOTIFY_INTERVAL_MS 30000  /* 告警推送间隔，避免刷屏 */

static uint32_t s_lastNotifyMs = 0;
static AlarmType s_lastAlarm = ALARM_NONE;

void alarm_task(void* pvParameters) {
    (void)pvParameters;
    pinMode(PIN_LED, OUTPUT);
    digitalWrite(PIN_LED, LOW);

    TickType_t lastWake = xTaskGetTickCount();
    const TickType_t period = pdMS_TO_TICKS(TASK_ALARM_PERIOD_MS);
    bool ledOn = false;
    uint32_t ledToggleMs = 0;
    uint32_t offMs = LED_OFF_SLOW_MS;

    for (;;) {
        vTaskDelayUntil(&lastWake, period);
        uint32_t now = millis();

        AlarmType alarm = sensor_getAlarm();
        PumpState pumpState = pump_getState();

        /* 根据状态选择 LED 闪烁模式 */
        if (alarm == ALARM_LOW_WATER || alarm == ALARM_OVERFLOW || pumpState == PUMP_DRY_RUN_FAULT || pumpState == PUMP_OVERFLOW_FAULT) {
            offMs = LED_OFF_FAST_MS;  /* 严重异常：快闪 */
        } else if (alarm == ALARM_TDS_HIGH) {
            offMs = LED_OFF_FAST_MS;  /* TDS 超标：快闪 */
        } else {
            offMs = LED_OFF_SLOW_MS;  /* 正常：慢闪 */
        }

        /* LED 闪烁 */
        if (ledOn) {
            if ((now - ledToggleMs) >= LED_ON_MS) {
                digitalWrite(PIN_LED, LOW);
                ledOn = false;
                ledToggleMs = now;
            }
        } else {
            if ((now - ledToggleMs) >= offMs) {
                digitalWrite(PIN_LED, HIGH);
                ledOn = true;
                ledToggleMs = now;
            }
        }

        /* 告警推送（间隔发送，避免刷屏） */
        if (alarm != ALARM_NONE || pumpState == PUMP_DRY_RUN_FAULT || pumpState == PUMP_OVERFLOW_FAULT) {
            if ((now - s_lastNotifyMs) >= NOTIFY_INTERVAL_MS && alarm != s_lastAlarm) {
                s_lastNotifyMs = now;
                s_lastAlarm = alarm;
                if (alarm == ALARM_LOW_WATER) {
                    blinker_requestNotify(BLINKER_NOTIFY_LOW_WATER);
                } else if (alarm == ALARM_OVERFLOW || pumpState == PUMP_OVERFLOW_FAULT) {
                    blinker_requestNotify(BLINKER_NOTIFY_OVERFLOW);
                } else if (alarm == ALARM_TDS_HIGH) {
                    blinker_requestNotify(BLINKER_NOTIFY_TDS);
                } else if (pumpState == PUMP_DRY_RUN_FAULT) {
                    blinker_requestNotify(BLINKER_NOTIFY_DRY_RUN);
                }
            }
        } else {
            s_lastAlarm = ALARM_NONE;
        }
    }
}
