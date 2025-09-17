#include "upload_manager.h"
#include "config.h"
#include "platform_packet.h"
#include "comm_manager.h"
#include "beijing_time_store.h"
#include <Arduino.h>

// 定时上传的计时器
static uint32_t lastRealtimeUploadMs = 0;

// 事件上传标志（在 main.ino 中定义，这里只声明使用）
extern volatile int g_monitorEventUploadFlag;

static void uploadRealtimeDataIfNeeded(uint32_t now) {
    if (!comm_isConnected()) return;
    if (now - lastRealtimeUploadMs < REALTIME_UPLOAD_INTERVAL_MS) return;

    // 读取当前北京时间
    BeijingDateTime bjt = getBeijingDateTime();
    uint16_t year = bjt.year;
    uint8_t year_high = (year >> 8) & 0xFF;
    uint8_t year_low  = year & 0xFF;

    // 按 platform_packet.h 的参数类型传递
    // void sendRealtimeMonitorData(uint8_t year_high, uint8_t year_low, uint8_t month, uint8_t day,
    //     uint8_t hour, uint8_t min, uint8_t sec, const uint8_t* exceptionStatus, uint8_t waterStatus);

    sendRealtimeMonitorData(
        year_high, year_low, bjt.month, bjt.day, bjt.hour, bjt.min, bjt.sec,
        nullptr,   // exceptionStatus
        0          // waterStatus
    );

    lastRealtimeUploadMs = now;
}

static void uploadMonitorEventIfNeeded() {
    if (!comm_isConnected()) return;
    if (g_monitorEventUploadFlag != 1) return;

    BeijingDateTime bjt = getBeijingDateTime();
    uint16_t year = bjt.year;
    uint8_t year_high = (year >> 8) & 0xFF;
    uint8_t year_low  = year & 0xFF;

    // 只传协议要求的参数顺序
    // void sendMonitorEventUpload(uint8_t year_high, uint8_t year_low, uint8_t month, uint8_t day,
    //     uint8_t hour, uint8_t min, uint8_t sec, float realtimeValue, float thresholdValue,
    //     const uint8_t* imageData, uint32_t imageLen);

    float realtimeValue = 12.34f;
    float thresholdValue = 10.00f;
    const uint8_t* imageData = nullptr;
    uint32_t imageLen = 0;

    sendMonitorEventUpload(
        year_high, year_low, bjt.month, bjt.day, bjt.hour, bjt.min, bjt.sec,
        realtimeValue, thresholdValue,
        imageData, imageLen
    );

    g_monitorEventUploadFlag = 0; // 上传一次后清零，防止重复
}

void upload_drive() {
    uint32_t now = millis();
    uploadRealtimeDataIfNeeded(now);
    uploadMonitorEventIfNeeded();
}