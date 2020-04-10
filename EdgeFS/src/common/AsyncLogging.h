#pragma once

#include "common.h"

class AsyncLogging
    : noncopyable
{
public:
    static void create();
    static void release();
    static AsyncLogging* instance();

public:
    AsyncLogging();
    ~AsyncLogging();

public:
    void init(const std::string& path);

    void start(); 
    void stop();

public:
    void log(const std::string& msg);
    
    LogLevel getLogLevel()
    {
        return m_loglevel;
    }

private:
    static void threadFunc(AsyncLogging* p);

    void logOutput();

private:
    std::string getCurrentTimeString();

private:
    static AsyncLogging*        m_pInstance;

    LogLevel                    m_loglevel;

    std::atomic<bool>           m_isStop;
    std::thread                 m_threadHandler;

    std::deque<std::string>     m_shareQueue;
    std::mutex                  m_MutexQueue;
    std::condition_variable     m_condQueue;

    FileOper*                   m_pFileOper;
};