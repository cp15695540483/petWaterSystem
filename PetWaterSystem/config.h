/**
 * @file config.h
 * @brief 智能宠物饮水系统 - 配置文件（引脚、阈值、WiFi、Blinker）
 * @note 使用前请修改 BLINKER_AUTH、WIFI_SSID、WIFI_PASSWORD
 */

#ifndef CONFIG_H
#define CONFIG_H

/* 启用则开启 Serial 调试（会增加约 20KB 固件，编译超限时请注释掉） */
#define DEBUG_SERIAL

/* ========== WiFi 与 Blinker 配置（必改） ========== */
#define BLINKER_AUTH    "YOUR_BLINKER_AUTH"
#define WIFI_SSID       "YOUR_WIFI_SSID"
#define WIFI_PASSWORD   "YOUR_WIFI_PASSWORD"

/* ========== 硬件引脚定义（ESP32-WROOM-DA） ========== */
/* TB6612 电机驱动模块引脚定义（驱动水泵）- 根据实际接线修改 */
#define PIN_PUMP_PWM     4    // TB6612 PWMA 输入（速度控制）
#define PIN_PUMP_AIN1    15   // TB6612 AIN1 输入（方向控制 1）
#define PIN_PUMP_AIN2    16   // TB6612 AIN2 输入（方向控制 2）
/* STBY 引脚直接接 3.3V，无需 GPIO 控制 */
#define PIN_WATER_LEVEL  34   // 雨水传感器 ADC（ADC1_CH6，仅输入）
#define PIN_TDS          35   // TDS 传感器 ADC（ADC1_CH7，仅输入）
#define PIN_BTN_EMERGENCY 13  // 紧急补水键（内部上拉，按下 LOW）
#define PIN_BTN_RESET    14   // 参数重置键
#define PIN_LED          2    // 状态 LED（经 220Ω 限流，高电平亮；部分板载 LED）

/* ========== ADC 参数（ESP32 12 位） ========== */
#define ADC_BITS         12
#define ADC_MAX          ((1 << ADC_BITS) - 1)  // 4095

/* ========== 水位阈值（0~100 百分比） ========== */
#define WATER_LEVEL_LOW   20   // 低于此值自动补水
#define WATER_LEVEL_HIGH  90   // 达到此值停泵（溢水保护）
#define WATER_LEVEL_CRITICAL 5 // 低于此值缺水告警

/* ========== TDS 阈值（ppm） ========== */
#define TDS_WARN          500  // 超过此值水质告警

/* ========== 水泵保护参数 ========== */
#define PUMP_DRY_RUN_SEC  15   // 空转保护：启动后此秒数内水位无上升则停机
#define PUMP_MIN_RUN_MS   3000 // 水泵最短运行时间（避免频繁启停）

/* ========== 滑动平均滤波窗口 ========== */
#define FILTER_WINDOW_WATER 12
#define FILTER_WINDOW_TDS   8

/* ========== FreeRTOS 任务参数 ========== */
#define TASK_PUMP_STACK    2048
#define TASK_PUMP_PRIO     3
#define TASK_PUMP_PERIOD_MS 10

#define TASK_SENSOR_STACK  2048
#define TASK_SENSOR_PRIO   2
#define TASK_SENSOR_PERIOD_MS 100

/* Blinker 已移至 loop 运行，以下参数已废弃 */
#define TASK_BLINKER_STACK 8192
#define TASK_BLINKER_PRIO  1
#define TASK_BLINKER_PERIOD_MS 500

#define TASK_ALARM_STACK   2048
#define TASK_ALARM_PRIO    0
#define TASK_ALARM_PERIOD_MS 50

/* 串口调试宏（仅当 DEBUG_SERIAL 启用时生效） */
#if defined(DEBUG_SERIAL)
#define DBG_PRINT(x)    Serial.print(x)
#define DBG_PRINTLN(x)  Serial.println(x)
#define DBG_PRINTF(...) Serial.printf(__VA_ARGS__)
#else
#define DBG_PRINT(x)
#define DBG_PRINTLN(x)
#define DBG_PRINTF(...)
#endif

#endif /* CONFIG_H */
