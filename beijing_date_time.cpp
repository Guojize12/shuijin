#include "beijing_date_time.h"
#include "beijing_time_store.h"

// 简单的日期工具
static bool is_leap(int y) {
    return ((y % 4 == 0 && y % 100 != 0) || (y % 400 == 0));
}
static int days_in_month(int y, int m) {
    static const int dim[2][12] = {
        {31,28,31,30,31,30,31,31,30,31,30,31},
        {31,29,31,30,31,30,31,31,30,31,30,31}
    };
    return dim[is_leap(y)][m - 1];
}

// 在 y-m-d h:m 基础上增加/减少分钟，规范化日期时间
static void add_minutes(int &year, int &month, int &day, int &hour, int &min, int deltaMin) {
    long totalMin = (long)hour * 60 + min + deltaMin;
    long dayShift = 0;

    // 规范化小时分钟并计算跨天
    if (totalMin >= 0) {
        dayShift = totalMin / (24 * 60);
        totalMin = totalMin % (24 * 60);
    } else {
        // 负数时向下取整
        dayShift = (totalMin - (24 * 60 - 1)) / (24 * 60);
        totalMin = totalMin - dayShift * (24 * 60);
    }

    hour = (int)(totalMin / 60);
    min  = (int)(totalMin % 60);

    // 调整日期
    if (dayShift != 0) {
        long d = dayShift;
        if (d > 0) {
            while (d-- > 0) {
                day++;
                if (day > days_in_month(year, month)) {
                    day = 1;
                    month++;
                    if (month > 12) {
                        month = 1;
                        year++;
                    }
                }
            }
        } else {
            while (d++ < 0) {
                day--;
                if (day < 1) {
                    month--;
                    if (month < 1) {
                        month = 12;
                        year--;
                    }
                    day = days_in_month(year, month);
                }
            }
        }
    }
}

// 解析 +CCLK: "25/09/17,02:43:22+32" -> 统一换算到北京时间
bool parseBeijingDateTime(const String& line, int &year, int &month, int &day, int &hour, int &min, int &sec) {
    int q1 = line.indexOf('\"');
    int q2 = line.lastIndexOf('\"');
    if (q1 < 0 || q2 <= q1) return false;
    String p = line.substring(q1 + 1, q2);

    int yy, MM, dd, hh, mm, ss, tzUnits = 0;
    char sign = '+';
    // tz单位为15分钟，例如+32 => +8小时；+34 => +8小时30分
    if (sscanf(p.c_str(), "%d/%d/%d,%d:%d:%d%c%d", &yy, &MM, &dd, &hh, &mm, &ss, &sign, &tzUnits) != 8) {
        return false;
    }

    // 把两位年转成四位年
    year  = yy + 2000;
    month = MM;
    day   = dd;
    hour  = hh;
    min   = mm;
    sec   = ss;

    // 1) AT返回的是“本地时间(由tz描述的时区)”。
    //    tzUnits单位=15分钟；先算出与UTC的偏移（分钟）
    int tzMinutes = (tzUnits / 4) * 60 + (tzUnits % 4) * 15;
    if (sign == '-') tzMinutes = -tzMinutes;

    // 2) 将“本地时间(AT)”换算为UTC：UTC = local - tz
    add_minutes(year, month, day, hour, min, -tzMinutes);

    // 3) 将UTC换算为北京时间(BJT = UTC + 8小时)
    add_minutes(year, month, day, hour, min, +8 * 60);

    // 记录到全局（内部始终以北京时间为准）
    setBeijingDateTime(year, month, day, hour, min, sec);
    return true;
}

void printBeijingDateTime(const String& line) {
    int year, month, day, hour, min, sec;
    if (parseBeijingDateTime(line, year, month, day, hour, min, sec)) {
        // 这里打印的是北京时间
        Serial2.printf("[北京时间] %04d-%02d-%02d %02d:%02d:%02d\r\n", year, month, day, hour, min, sec);
    }
}