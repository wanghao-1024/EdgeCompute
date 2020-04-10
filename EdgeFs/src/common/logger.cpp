#include "./common.h"

#define LOG_MAX_BUF_SIZE 2048

void mediaLog(LogLevel level, const char* level_str, const char* file, const char* format, ...)
{
    if (!AsyncLogging::instance())
    {
        return ;
    }

    LogLevel curLogLevel = AsyncLogging::instance()->getLogLevel();
    if (level == LogLevel_NONE || (uint32_t)level < (uint32_t)curLogLevel)
    {
        return;
    }

    char buf[LOG_MAX_BUF_SIZE] = { 0 };
    va_list args;

#ifdef IOS
    //int tid = syscall(SYS_gettid);//always return -1 in iOS
    //int tid = -1;
    //snprintf(buf, 20, "[%d] ", tid);
#else
    //snprintf(buf, 30, "[%d:%p] ", syscall(SYS_gettid), (void*)pthread_self());
#endif
    uint32_t uLen = (uint32_t)strlen(buf);

    const char* fileName = strrchr(file, '/');
    if (fileName == NULL)
    {
        fileName = file;
    }
    else
    {
        fileName++;
    }
    snprintf((char*)(buf + uLen), 30, "%s [%s", level_str, fileName);
    uLen = (uint32_t)strlen(buf);

    va_start(args, format);
    vsnprintf((char*)(buf + uLen), (LOG_MAX_BUF_SIZE - 1 - uLen), format, args);
    va_end(args);

    buf[LOG_MAX_BUF_SIZE - 1] = 0;

    AsyncLogging::instance()->log(buf);
}