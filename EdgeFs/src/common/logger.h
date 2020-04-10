#pragma once
#include "./common.h"

enum LogLevel
{
    LogLevel_DEBUG,
    LogLevel_INFO,
    LogLevel_NOTICE,    // 用户时间的通知
    LogLevel_WARN,
    LogLevel_ERROR,
    LogLevel_FATAL,     // 致命告警，出现则应该立即停止运行
    LogLevel_NONE,      // 不输出日志
};

void mediaLog(LogLevel level, const char* level_str, const char* file, const char* format, ...) __attribute__((format(printf,4,5)));

#if 1
#define logger(level, level_str, format, args...) \
        do { mediaLog(level, level_str, __FILE__, "-%s:%d] " format, __FUNCTION__, __LINE__, ##args); } while(0)
#else
#define logger(level, level_str, format, args...)   \
        do { mediaLog(level, "%s " format, level_str, ##args); } while(0)
#endif

#define  ldebug(format, args...)        logger(LogLevel_DEBUG,  "D", format, ##args)
#define  linfo(format, args...)         logger(LogLevel_INFO,   "I", format, ##args)
#define  lnotice(format, args...)       logger(LogLevel_NOTICE, "N", format, ##args)
#define  lwarn(format, args...)         logger(LogLevel_WARN,   "W", format, ##args)
#define  lerror(format, args...)        logger(LogLevel_ERROR,  "E", format, ##args)
#define  lfatal(format, args...)        logger(LogLevel_FATAL,  "F", format, ##args)

