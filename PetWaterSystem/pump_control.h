/**
 * @file pump_control.h
 * @brief 水泵控制与保护（空转/溢水/手动/自动/定时）
 */

#ifndef PUMP_CONTROL_H
#define PUMP_CONTROL_H

#include <stdint.h>
#include <stdbool.h>  /* C++ 下可用，bool 为基本类型 */

#ifdef __cplusplus
extern "C" {
#endif

/** 水泵运行状态 */
typedef enum {
    PUMP_OFF,
    PUMP_RUNNING,
    PUMP_DRY_RUN_FAULT,
    PUMP_OVERFLOW_FAULT
} PumpState;

/**
 * @brief 初始化水泵控制（GPIO、状态）
 */
void pump_init(void);

/**
 * @brief 设置水位百分比（由 sensor_task 调用，0~100）
 */
void pump_setWaterLevel(float level);

/**
 * @brief 获取水泵是否正在运行
 */
bool pump_isRunning(void);

/**
 * @brief 获取水泵状态
 */
PumpState pump_getState(void);

/**
 * @brief 请求手动补水（本地按键或 Blinker 远程）
 * @param duration_ms 0 表示自动补水直到水位达标
 */
void pump_requestManualFill(uint32_t duration_ms);

/**
 * @brief 请求定时补水（Blinker 定时组件）
 * @param enable 1 启动，0 停止
 * @param duration_ms 本次补水时长
 */
void pump_requestTimedFill(bool enable, uint32_t duration_ms);

/**
 * @brief 清除故障标志（参数重置键）
 */
void pump_clearFault(void);

/**
 * @brief 水泵控制任务入口（FreeRTOS 任务函数）
 */
void pump_task(void* pvParameters);

#ifdef __cplusplus
}
#endif

#endif /* PUMP_CONTROL_H */
