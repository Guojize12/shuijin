#include "upload_manager.h"
#include "config.h"
#include "platform_packet.h"
#include "comm_manager.h"
#include "rtc_soft.h"
#include "sd_async.h"
#include <Arduino.h>
#include <FS.h>
#include <SD.h>

// 定时上传的计时器
static uint32_t lastRealtimeUploadMs = 0;

// 事件上传标志（在 main.ino 中定义，这里只声明使用）
extern volatile int g_monitorEventUploadFlag;

// 最后一张照片文件名（由 capture_trigger 维护）
extern char g_lastPhotoName[64];

// ============ 新增：只上报一次开机状态 ============
static bool g_startupReported = false;

static bool g_simInfoUploaded = false;

static void uploadSimInfoIfNeeded() {
    if (g_simInfoUploaded) return;
    if (!comm_isConnected()) {
        return;
    }
    if (!rtc_is_valid()) {
        log2("[SIMUP] RTC not valid, skip.");
        return;
    }

    log2("[SIMUP] Try collect SIM info");
    SimInfo sim;
    if (!siminfo_query(&sim)) {
        log2("[SIMUP] SIM info collect fail, skip upload.");
        return;
    }

    PlatformTime t;
    rtc_now_fields(&t);

    log2Str("[SIMUP] Ready upload ICCID: ", sim.iccid);
    log2Str("[SIMUP] Ready upload IMSI: ", sim.imsi);

    sendSimInfoUpload(
        t.year, t.month, t.day, t.hour, t.minute, t.second,
        sim.iccid, sim.iccid_len,
        sim.imsi,  sim.imsi_len,
        sim.signal
    );
    log2("[SIMUP] SIM info upload sent.");
    g_simInfoUploaded = true;
}

// 开机自动上报（只上报一次，校时+联网后）
static void uploadStartupStatusIfNeeded() {
    if (g_startupReported) return;
    if (!comm_isConnected()) return;
    if (!rtc_is_valid()) return;

    PlatformTime t;
    rtc_now_fields(&t);

    // CMD=0x0002开机状态上报
    sendStartupStatusReport(
        t.year, t.month, t.day, t.hour, t.minute, t.second,
        0 // 默认状态字0
    );
    g_startupReported = true;
}

static void uploadRealtimeDataIfNeeded(uint32_t now) {
    if (!comm_isConnected()) return;
    if (!rtc_is_valid()) {
        return;
    }
    if (now - lastRealtimeUploadMs < REALTIME_UPLOAD_INTERVAL_MS) return;

    PlatformTime t;
    rtc_now_fields(&t);

    sendRealtimeMonitorData(
        t.year, t.month, t.day, t.hour, t.minute, t.second, // 用2字节year
        0,
        nullptr,
        0
    );

    lastRealtimeUploadMs = now;
}

// 将 g_lastPhotoName 指向的文件读取到内存（≤65000），成功返回malloc的指针与长度
static uint8_t* read_photo_into_ram(size_t& outLen) {
    outLen = 0;
    if (!g_lastPhotoName[0]) return nullptr;

    // 如果启用异步写，且还未空闲，则暂缓上传，等下一轮
    if (g_cfg.asyncSDWrite && !sd_async_idle()) {
        return nullptr;
    }

    File f = SD.open(g_lastPhotoName, FILE_READ);
    if (!f) {
        Serial.println("[UPLOAD] Photo file open failed!");
        return nullptr;
    }
    size_t sz = f.size();
    if (sz == 0) {
        f.close();
        Serial.println("[UPLOAD] Photo file size=0!");
        return nullptr;
    }
    if (sz > 65000) {
        f.close();
        Serial.println("[UPLOAD] Photo too large (>65K), skip upload.");
        return nullptr;
    }
    uint8_t* buf = (uint8_t*)malloc(sz);
    if (!buf) {
        f.close();
        Serial.println("[UPLOAD] malloc failed for photo buffer!");
        return nullptr;
    }
    size_t n = f.read(buf, sz);
    f.close();
    if (n != sz) {
        free(buf);
        Serial.println("[UPLOAD] Photo read size mismatch!");
        return nullptr;
    }
    outLen = sz;
    return buf;
}

static void uploadMonitorEventIfNeeded() {
    if (!comm_isConnected()) return;
    if (!rtc_is_valid()) {
        return;
    }
    if (g_monitorEventUploadFlag != 1) return;

    // 读取图片数据
    size_t imgLen = 0;
    uint8_t* imageData = read_photo_into_ram(imgLen);
    if (!imageData && g_cfg.asyncSDWrite) {
        // 异步未空闲，或读取失败，下一轮再试（不清标志）
        return;
    }

    PlatformTime t;
    rtc_now_fields(&t);

    uint8_t triggerCond = 1;
    float realtimeValue = 0.0f;   // 可按需填写
    float thresholdValue = 0.0f;  // 可按需填写

    // 如果没读到图片（可能是异步未完成、或大于65K、或其它失败），按需求：可只发元数据，或跳过
    if (imageData && imgLen > 0 && imgLen <= 65000) {
        sendMonitorEventUpload(
            t.year, t.month, t.day, t.hour, t.minute, t.second, triggerCond,
            realtimeValue, thresholdValue, imageData, (uint32_t)imgLen
        );
        free(imageData);
    } else {
        Serial.println("[UPLOAD] No image attached (either too large or not ready). Send meta only.");
        sendMonitorEventUpload(
            t.year, t.month, t.day, t.hour, t.minute, t.second, triggerCond,
            realtimeValue, thresholdValue, nullptr, 0
        );
    }

    // 按你的要求：上传成功不删除本地文件，这里不做删除
    g_monitorEventUploadFlag = 0; // 上传一次后清零
}

void upload_drive() {
    uint32_t now = millis();
    uploadStartupStatusIfNeeded();     // 开机状态上报
    uploadSimInfoIfNeeded();           // SIM卡状态上报
    uploadRealtimeDataIfNeeded(now);   // 实时数据上报
    uploadMonitorEventIfNeeded();      // 事件图片上传
}