#include "beijing_time_store.h"

static BeijingDateTime g_beijingDateTime = {2000, 1, 1, 0, 0, 0};

static bool is_leap(int y) {
    return ((y % 4 == 0 && y % 100 != 0) || (y % 400 == 0));
}

void setBeijingDateTime(int year, int month, int day, int hour, int min, int sec) {
    g_beijingDateTime.year  = year;
    g_beijingDateTime.month = month;
    g_beijingDateTime.day   = day;
    g_beijingDateTime.hour  = hour;
    g_beijingDateTime.min   = min;
    g_beijingDateTime.sec   = sec;
}

BeijingDateTime getBeijingDateTime(void) {
    return g_beijingDateTime;
}

void rtc_tick(void) {
    static const int dim[2][12] = {
        {31,28,31,30,31,30,31,31,30,31,30,31},
        {31,29,31,30,31,30,31,31,30,31,30,31}
    };

    g_beijingDateTime.sec++;
    if (g_beijingDateTime.sec >= 60) {
        g_beijingDateTime.sec = 0;
        g_beijingDateTime.min++;
        if (g_beijingDateTime.min >= 60) {
            g_beijingDateTime.min = 0;
            g_beijingDateTime.hour++;
            if (g_beijingDateTime.hour >= 24) {
                g_beijingDateTime.hour = 0;
                g_beijingDateTime.day++;
                int leap = is_leap(g_beijingDateTime.year);
                if (g_beijingDateTime.day > dim[leap][g_beijingDateTime.month - 1]) {
                    g_beijingDateTime.day = 1;
                    g_beijingDateTime.month++;
                    if (g_beijingDateTime.month > 12) {
                        g_beijingDateTime.month = 1;
                        g_beijingDateTime.year++;
                    }
                }
            }
        }
    }
}