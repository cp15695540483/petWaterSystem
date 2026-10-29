/**
 * @file pump_control.cpp
 * @brief 水泵控制与空转/溢水保护实现（TB6612电机驱动模块）
 */

#include "config.h"
#include "pump_control.h"
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

/* 共享变量（由 sensor_task 更新） */
static volatile float s_waterLevel = 0;

/* 控制状态 */
static volatile bool s_pumpOn = false;
static volatile PumpState s_state = PUMP_OFF;
static volatile bool s_manualRequest = false;
static volatile uint32_t s_manualDurationMs = 0;
static volatile bool s_timedFillActive = false;
static volatile uint32_t s_timedRemainMs = 0;

static uint32_t s_pumpStartMs = 0;
static float s_waterLevelAtStart = 0;

void pump_init(void) {
    /* TB6612 引脚初始化 */
    pinMode(PIN_PUMP_PWM, OUTPUT);
    pinMode(PIN_PUMP_AIN1, OUTPUT);
    pinMode(PIN_PUMP_AIN2, OUTPUT);
    
    /* 初始状态：TB6612 已使能（STBY 接 3.3V），电机停止 */
    digitalWrite(PIN_PUMP_AIN1, LOW);   // AIN1 低电平
    digitalWrite(PIN_PUMP_AIN2, LOW);   // AIN2 低电平
    digitalWrite(PIN_PUMP_PWM, LOW);    // PWM 低电平，电机停止

    s_pumpOn = false;
    s_state = PUMP_OFF;
    s_manualRequest = false;
    s_timedFillActive = false;
#if defined(DEBUG_SERIAL)
    DBG_PRINTLN(F("[水泵] TB6612初始化完成"));
#endif
}

void pump_setWaterLevel(float level) {
    s_waterLevel = level;
}

bool pump_isRunning(void) {
    return s_pumpOn;
}

PumpState pump_getState(void) {
    return s_state;
}

void pump_requestManualFill(uint32_t duration_ms) {
    s_manualRequest = true;
    s_manualDurationMs = duration_ms;
}

void pump_requestTimedFill(bool enable, uint32_t duration_ms) {
    s_timedFillActive = enable;
    if (enable) {
        s_timedRemainMs = duration_ms;
    }
}

void pump_clearFault(void) {
    if (s_state == PUMP_DRY_RUN_FAULT || s_state == PUMP_OVERFLOW_FAULT) {
        s_state = PUMP_OFF;
    }
}

void pump_task(void* pvParameters) {
    (void)pvParameters;
    TickType_t lastWake = xTaskGetTickCount();
    const TickType_t period = pdMS_TO_TICKS(TASK_PUMP_PERIOD_MS);

    for (;;) {
        vTaskDelayUntil(&lastWake, period);

        float level = s_waterLevel;
        uint32_t now = millis();

        /* 故障时仅响应清除，除非有手动补水请求 */
        if (s_state == PUMP_DRY_RUN_FAULT || s_state == PUMP_OVERFLOW_FAULT) {
            if (!s_manualRequest) {
                digitalWrite(PIN_PUMP_PWM, LOW);  // TB6612停止输出
                s_pumpOn = false;
                continue;
            } else {
                // 手动补水请求覆盖故障状态
                s_state = PUMP_OFF;
            }
        }

        /* 溢水保护 */
        if (level >= WATER_LEVEL_HIGH) {
            digitalWrite(PIN_PUMP_PWM, LOW);  // TB6612停止输出
            s_pumpOn = false;
            s_state = PUMP_OVERFLOW_FAULT;
#if defined(DEBUG_SERIAL)
            DBG_PRINTLN(F("[水泵] 溢水保护: 水位过高，已停机"));
#endif
            s_manualRequest = false;
            s_timedFillActive = false;
            continue;
        }

        /* 判断是否需要补水 */
        bool needFill = false;

        if (s_manualRequest) {
            needFill = true;
            if (s_manualDurationMs > 0) {
                uint32_t elapsed = now - s_pumpStartMs;
                if (elapsed >= s_manualDurationMs) {
                    s_manualRequest = false;
                    needFill = false;
                }
            } else {
                // 移除水位检查，允许用户在任何水位下手动补水
                // 仅在达到溢水保护水位时停止
                if (level >= WATER_LEVEL_HIGH) {
                    s_manualRequest = false;
                    needFill = false;
                }
            }
        }

        if (s_timedFillActive && s_timedRemainMs > 0) {
            needFill = true;
            s_timedRemainMs -= TASK_PUMP_PERIOD_MS;
            if (s_timedRemainMs == 0) s_timedRemainMs = 1; /* 本周期内仍运行 */
            if (s_timedRemainMs <= TASK_PUMP_PERIOD_MS) {
                s_timedRemainMs = 0;
                s_timedFillActive = false;
            }
        }

        if (!needFill && level < WATER_LEVEL_LOW) {
            needFill = true; /* 自动补水 */
        }

        /* 执行启停 */
        if (needFill) {
            if (!s_pumpOn) {
                s_pumpStartMs = now;
                s_waterLevelAtStart = level;
            }
         /* TB6612 启动电机：AIN1 高电平，AIN2 低电平，PWM 高电平（全速） */
             digitalWrite(PIN_PUMP_AIN1, HIGH);  // AIN1 高电平
             digitalWrite(PIN_PUMP_AIN2, LOW);   // AIN2 低电平
             digitalWrite(PIN_PUMP_PWM, HIGH);   // PWM 高电平，全速运行
            s_pumpOn = true;
            s_state = PUMP_RUNNING;

            /* 空转保护 */
            uint32_t runTime = (now - s_pumpStartMs) / 1000;
            if (runTime >= PUMP_DRY_RUN_SEC) {
                float rise = level - s_waterLevelAtStart;
                if (rise < 2.0f) {
                    digitalWrite(PIN_PUMP_PWM, LOW);  // TB6612停止输出
                    s_pumpOn = false;
                    s_state = PUMP_DRY_RUN_FAULT;
#if defined(DEBUG_SERIAL)
                    DBG_PRINTLN(F("[水泵] 空转保护: 运行超时水位无上升，已停机"));
#endif
                    s_manualRequest = false;
                    s_timedFillActive = false;
                }
            }
        } else {
            /* 最短运行时间保护：运行不足则继续 */
            if (s_pumpOn) {
                uint32_t runMs = now - s_pumpStartMs;
                if (runMs >= PUMP_MIN_RUN_MS) {
                    digitalWrite(PIN_PUMP_PWM, LOW);  // TB6612停止输出
                    s_pumpOn = false;
                    s_state = PUMP_OFF;
                }
            } else {
                s_state = PUMP_OFF;
            }
        }
    }
}
