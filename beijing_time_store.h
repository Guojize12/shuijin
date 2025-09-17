#ifndef _BEIJING_TIME_STORE_H_
#define _BEIJING_TIME_STORE_H_

typedef struct {
    int year, month, day, hour, min, sec; // 全部为北京时间
} BeijingDateTime;

void setBeijingDateTime(int year, int month, int day, int hour, int min, int sec);
BeijingDateTime getBeijingDateTime(void);

// 每秒调用一次，按北京时间自增
void rtc_tick(void);

#endif