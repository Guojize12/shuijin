#pragma once
#pragma once

void startATPing();
void queryCEREG();
void setEncoding();
void closeCh0();
void openTCP();
void pollMIPSTATE();

// 新增：获取模块时钟
void queryCCLK();