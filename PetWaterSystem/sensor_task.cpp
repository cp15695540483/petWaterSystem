/**
 * @file sensor_task.cpp
 * @brief 水位与 TDS 采集、滑动平均滤波、告警检测
 */

#include "config.h"
#include "sensor_task.h"
#include "pump_control.h"
#include "filters.h"
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

/* TDS 标定系数（根据实际模块调整） */
#define TDS_K  (0.5f)  /* 经验系数 */

static MovingAverageFilter* s_waterFilter = nullptr;
static MovingAverageFilter* s_tdsFilter   = nullptr;

static volatile float s_waterLevel = 0;
static volatile int   s_tdsValue   = 0;
static volatile AlarmType s_alarm  = ALARM_NONE;

float sensor_getWaterLevel(void) {
    return s_waterLevel;
}

int sensor_getTDS(void) {
    return s_tdsValue;
}

AlarmType sensor_getAlarm(void) {
    return s_alarm;
}

void sensor_task(void* pvParameters) {
    (void)pvParameters;
    s_waterFilter = new MovingAverageFilter(FILTER_WINDOW_WATER);
    s_tdsFilter   = new MovingAverageFilter(FILTER_WINDOW_TDS);

    /* ESP32 ADC 衰减 11dB，量程约 0~3.3V（WiFi 开启时仅 ADC1 可用） */
    analogReadResolution(ADC_BITS);
    analogSetAttenuation(ADC_11db);

    TickType_t lastWake = xTaskGetTickCount();
    const TickType_t period = pdMS_TO_TICKS(TASK_SENSOR_PERIOD_MS);

    for (;;) {
        vTaskDelayUntil(&lastWake, period);

        /* 读取原始 ADC（水位：空=低水位=低 ADC，满=高 ADC） */
        int rawWater = analogRead(PIN_WATER_LEVEL);
        int rawTDS   = analogRead(PIN_TDS);

        /* 滑动平均滤波 */
        float waterFiltered = s_waterFilter->update((float)rawWater);
        float tdsFiltered   = s_tdsFilter->update((float)rawTDS);

        /* 水位百分比：ADC 越高水位越低（干燥时ADC高），映射到 0~100 */
        float level = 100.0f - (waterFiltered / ADC_MAX) * 100.0f;
        if (level < 0) level = 0;
        if (level > 100) level = 100;
        s_waterLevel = level;

        /* TDS：ADC 电压 * 系数 得到 ppm 近似值（需根据实际传感器标定） */
        float voltage = (tdsFiltered / ADC_MAX) * 3.3f;
        int tds = (int)(voltage * TDS_K * 1000);
        if (tds < 0) tds = 0;
        s_tdsValue = tds;

        /* 同步给水泵控制 */
        pump_setWaterLevel(s_waterLevel);

        /* 告警检测 */
        if (level <= WATER_LEVEL_CRITICAL) {
            s_alarm = ALARM_LOW_WATER;
        } else if (level >= WATER_LEVEL_HIGH) {
            s_alarm = ALARM_OVERFLOW;
        } else if (tds >= TDS_WARN) {
            s_alarm = ALARM_TDS_HIGH;
        } else {
            /* 溢水故障由 pump 单独处理，此处仅根据水位 */
            if (pump_getState() == PUMP_OVERFLOW_FAULT) {
                s_alarm = ALARM_OVERFLOW;
            } else if (s_alarm != ALARM_OVERFLOW) {
                s_alarm = ALARM_NONE;
            }
        }
    }
}
