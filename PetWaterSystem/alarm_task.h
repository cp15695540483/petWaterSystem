/**
 * @file alarm_task.h
 * @brief 告警与状态指示任务 - LED 闪烁、异常告警
 */

#ifndef ALARM_TASK_H
#define ALARM_TASK_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 告警与 LED 状态指示任务入口（FreeRTOS 任务函数）
 */
void alarm_task(void* pvParameters);

#ifdef __cplusplus
}
#endif

#endif /* ALARM_TASK_H */
