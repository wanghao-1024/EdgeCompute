#include "common.h"

AsyncLogging* AsyncLogging::m_pInstance = NULL;

void AsyncLogging::create()
{
    if (NULL == m_pInstance)
    {
        m_pInstance = new AsyncLogging();
    }
}

void AsyncLogging::release()
{
    if (NULL != m_pInstance)
    {
        delete m_pInstance;
        m_pInstance = NULL;
    }
}

AsyncLogging* AsyncLogging::instance()
{
    return m_pInstance;
}

AsyncLogging::AsyncLogging()
: m_loglevel(LogLevel_INFO)
, m_isStop(true)
{
    m_pFileOper = new FileOper();
    start();
}

AsyncLogging::~AsyncLogging()
{
    stop();
    m_pFileOper->close();
    SAFE_DELETE(m_pFileOper);
}

void AsyncLogging::init(const std::string& path)
{
    m_pFileOper->setPath(path);
    m_pFileOper->open(O_RDWR | O_CREAT | O_APPEND);
}

void AsyncLogging::start()
{
    if (!m_isStop)
    {
        return ;
    }
    m_isStop = false;
    m_threadHandler = std::thread(AsyncLogging::threadFunc, this);
}

void AsyncLogging::stop()
{
    if (m_isStop)
    {
        return ;
    }
    m_isStop = true;
    m_condQueue.notify_one();
    m_threadHandler.join();
}

void AsyncLogging::log( const std::string& msg )
{
    if (m_shareQueue.size() > 1000)
    {
        return;
    }

    std::string info = getCurrentTimeString() + " ";
    info.append(msg);
    info.append("\n");

    {
        std::unique_lock<std::mutex> lock(m_MutexQueue);
        m_shareQueue.push_back(info);
    }
    m_condQueue.notify_one();
}

void AsyncLogging::threadFunc(AsyncLogging* p)
{
    p->logOutput();
}

void AsyncLogging::logOutput()
{
    while(!m_isStop)
    {
        std::deque<std::string> logQ;
        {
            std::unique_lock<std::mutex> lock(m_MutexQueue);
            if (m_shareQueue.empty())
            {
                m_condQueue.wait(lock);
            }
            logQ.swap(m_shareQueue);
        }

        for (auto && msg : logQ)
        {
            m_pFileOper->write(msg.c_str(), msg.size());
        }
    }
}

std::string AsyncLogging::getCurrentTimeString()
{
    struct timeval curTime;
    gettimeofday(&curTime, NULL);

    char buf[100]  = { '\0' };
    strftime(buf, sizeof(buf), "%F %T", localtime(&curTime.tv_sec));

    std::stringstream os;
    os << buf << "." << (uint32_t)(curTime.tv_usec / 1000);
    return os.str();
}