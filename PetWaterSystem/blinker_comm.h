/**
 * @file blinker_comm.h
 * @brief Blinker 物联网通信 - 数据上报、远程控制、告警推送
 * @note 需在 Arduino IDE 中安装 Blinker 库，并在 config.h 配置 WiFi 与密钥
 */

#ifndef BLINKER_COMM_H
#define BLINKER_COMM_H

#ifdef __cplusplus
extern "C" {
#endif

/** 告警推送类型（由 alarm_task 请求，loop 中 blinker_processNotify 执行，减轻任务栈压力） */
typedef enum {
    BLINKER_NOTIFY_NONE = 0,
    BLINKER_NOTIFY_LOW_WATER,
    BLINKER_NOTIFY_OVERFLOW,
    BLINKER_NOTIFY_TDS,
    BLINKER_NOTIFY_DRY_RUN
} BlinkerNotifyType;

/**
 * @brief 初始化 Blinker（WiFi 连接、回调注册）
 */
void blinker_init(void);

/**
 * @brief 数据上报（水位、TDS、水泵状态）
 * @note 由 loop 周期性调用
 */
void blinker_reportData(void);

/**
 * @brief 推送告警到 APP（直接调用 Blinker.notify，栈较大时慎用）
 * @param msg 告警消息
 */
void blinker_notify(const char* msg);

/**
 * @brief 请求推送告警（可在任意任务中调用；实际 notify 在 loop 的 blinker_processNotify 中执行）
 */
void blinker_requestNotify(BlinkerNotifyType type);

/**
 * @brief 处理待推送告警（须在 loop 中调用）
 */
void blinker_processNotify(void);

/**
 * @brief 驱动 Blinker 通信（须在 loop 中频繁调用，维持 MQTT 心跳）
 */
void blinker_run(void);

#ifdef __cplusplus
}
#endif

#endif /* BLINKER_COMM_H */
