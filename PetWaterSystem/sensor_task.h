/**
 * @file sensor_task.h
 * @brief 传感器采集任务 - 水位、TDS 采集与滤波
 */

#ifndef SENSOR_TASK_H
#define SENSOR_TASK_H

#ifdef __cplusplus
extern "C" {
#endif

/** 告警类型（供 alarm_task 使用） */
typedef enum {
    ALARM_NONE = 0,
    ALARM_LOW_WATER,    /* 缺水 */
    ALARM_OVERFLOW,     /* 溢水（由 pump 触发） */
    ALARM_TDS_HIGH      /* TDS 超标 */
} AlarmType;

/**
 * @brief 获取当前水位百分比 0~100
 */
float sensor_getWaterLevel(void);

/**
 * @brief 获取当前 TDS 值 ppm
 */
int sensor_getTDS(void);

/**
 * @brief 获取当前告警类型
 */
AlarmType sensor_getAlarm(void);

/**
 * @brief 传感器采集任务入口（FreeRTOS 任务函数）
 */
void sensor_task(void* pvParameters);

#ifdef __cplusplus
}
#endif

#endif /* SENSOR_TASK_H */
