#pragma once
#include <Arduino.h>

// 解析 +CCLK: "YY/MM/DD,hh:mm:ss±tz"（tz为每15分钟一个单位，例如+32=+8小时）
// 并转换为北京时间，写入 setBeijingDateTime。
// 同时返回转换后的北京时间(年-月-日 时:分:秒)。成功返回true。
bool parseBeijingDateTime(const String& line, int &year, int &month, int &day, int &hour, int &min, int &sec);

// 调试打印（北京时间）
void printBeijingDateTime(const String& line);