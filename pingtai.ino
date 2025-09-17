#include <Arduino.h>
#include "config.h"
#include "uart_utils.h"
#include "state_machine.h"
#include "platform_packet.h"
#include "at_commands.h"
#include "beijing_time_store.h" // 软件RTC（北京时间）
#include "comm_manager.h"

volatile int g_monitorEventUploadFlag = 0;

unsigned long lastTick = 0;
unsigned long lastSync = 0;
const unsigned long SYNC_INTERVAL = 3600000UL; // 每1小时校时一次
extern bool waitingForCCLK;
bool hasDoneFirstSync = false;  // 是否已完成本次连接的首次校时

void setup() {
  Serial.begin(DTU_BAUD);
  Serial2.begin(LOG_BAUD, SERIAL_8N1, RX2, TX2);

  log2("Boot");
  log2("UART0 for DTU, UART2 for log");
  log2Str("Server: ", SERVER_IP);
  log2Val("Port: ", SERVER_PORT);

  comm_resetBackoff();
  comm_gotoStep(STEP_IDLE);

  lastSync = millis();
}

void loop() 
{
  // 软件RTC按北京时间每秒自增
  if (millis() - lastTick >= 1000) {
    lastTick += 1000;
    rtc_tick();
  }

  // 只有通信建立后才发起AT+CCLK校时
  if (comm_isConnected()) {
    // 若首次连接，立即校时
    if (!hasDoneFirstSync && !waitingForCCLK) {
      queryCCLK();
      waitingForCCLK = true;
      hasDoneFirstSync = true;
      lastSync = millis(); // 避免后面又立刻校时
    }
    // 后续每1小时校时
    else if (!waitingForCCLK && (millis() - lastSync >= SYNC_INTERVAL)) {
      lastSync = millis();
      queryCCLK();
      waitingForCCLK = true;
    }
  } else {
    // 通信断开后，下次连接可以立即校时
    hasDoneFirstSync = false;
    waitingForCCLK = false;
  }

  // 主通信驱动，维持状态机和心跳
  comm_drive();

  // 业务流程（如上传等）
  driveStateMachine();
}