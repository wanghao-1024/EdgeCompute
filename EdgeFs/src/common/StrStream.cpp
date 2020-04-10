#include "StrStream.h"
#include <stdio.h>
#include <string.h>
#include "BufferPool.h"

namespace apd_vp2p
{
using std::string;

const uint32_t kMaxLogSize = 2048;

StrStream::StrStream()
    : m_data(NULL)
    , m_size(0)
    , m_maxSize(kMaxLogSize)
{
    m_data = (char*) BufferPool::allocBuff(m_maxSize);
    if (m_data)
    {
        m_data[0] = 0;
    }
}

StrStream::StrStream(uint32_t maxSize)
    : m_data(NULL)
    , m_size(0)
    , m_maxSize(maxSize)
{
    m_data = (char*)BufferPool::allocBuff(m_maxSize);
    if (m_data)
    {
        m_data[0] = 0;
    }
}

StrStream::~StrStream()
{
    BufferPool::freeBuff(m_data);
}

StrStream& StrStream::operator << (uint8_t val)
{
    if (m_data)
    {
        int n = snprintf(m_data + m_size, (m_maxSize - m_size), "%u", (uint32_t) val);
        if (n > 0)
        {
            m_size += n;
            if (m_size >= m_maxSize)
            {
                m_size = m_maxSize;
                m_data[m_size - 1] = 0;
            }
        }
    }

    return *this;
}

StrStream& StrStream::operator << (uint16_t val)
{
    if (m_data)
    {
        int n = snprintf(m_data + m_size, (m_maxSize - m_size), "%u", (uint32_t) val);
        if (n > 0)
        {
            m_size += n;
            if (m_size >= m_maxSize)
            {
                m_size = m_maxSize;
                m_data[m_size - 1] = 0;
            }
        }
    }

    return *this;
}

StrStream& StrStream::operator << (uint32_t val)
{
    if (m_data)
    {
        int n = snprintf(m_data + m_size, (m_maxSize - m_size), "%u", val);
        if (n > 0)
        {
            m_size += n;
            if (m_size >= m_maxSize)
            {
                m_size = m_maxSize;
                m_data[m_size - 1] = 0;
            }
        }
    }

    return *this;
}

StrStream& StrStream::operator << (uint64_t val)
{
    if (m_data)
    {
        int n = snprintf(m_data + m_size, (m_maxSize - m_size), "%llu", (long long unsigned int)val);
        if (n > 0)
        {
            m_size += n;
            if (m_size >= m_maxSize)
            {
                m_size = m_maxSize;
                m_data[m_size - 1] = 0;
            }
        }
    }

    return *this;
}

#ifdef __APPLE__
StrStream& StrStream::operator << (size_t val)
{
    if (m_data)
    {
        int n = snprintf(m_data + m_size, (m_maxSize - m_size), "%llu", (uint64_t)val);
        if (n > 0)
        {
            m_size += n;
            if (m_size >= m_maxSize)
            {
                m_size = m_maxSize;
                m_data[m_size - 1] = 0;
            }
        }
    }

    return *this;
}
#endif

StrStream& StrStream::operator << (int64_t val)
{
    if (m_data)
    {
        int n = snprintf(m_data + m_size, (m_maxSize - m_size), "%lld", (long long signed int)val);
        if (n > 0)
        {
            m_size += n;
            if (m_size >= m_maxSize)
            {
                m_size = m_maxSize;
                m_data[m_size - 1] = 0;
            }
        }
    }

    return *this;
}

StrStream& StrStream::operator << (int val)
{
    if (m_data)
    {
        int n = snprintf(m_data + m_size, (m_maxSize - m_size), "%d", val);
        if (n > 0)
        {
            m_size += n;
            if (m_size >= m_maxSize)
            {
                m_size = m_maxSize;
                m_data[m_size - 1] = 0;
            }
        }
    }

    return *this;
}

StrStream& StrStream::operator << (const string& str)
{
    if (m_data)
    {
        int n = snprintf(m_data + m_size, (m_maxSize - m_size), "%s", str.c_str());
        if (n > 0)
        {
            m_size += n;
            if (m_size >= m_maxSize)
            {
                m_size = m_maxSize;
                m_data[m_size - 1] = 0;
            }
        }
    }

    return *this;
}

StrStream& StrStream::operator << (const char* val)
{
    if (m_data)
    {
        int n = snprintf(m_data + m_size, (m_maxSize - m_size), "%s", val);
        if (n > 0)
        {
            m_size += n;
            if (m_size >= m_maxSize)
            {
                m_size = m_maxSize;
                m_data[m_size - 1] = 0;
            }
        }
    }

    return *this;
}

StrStream& StrStream::operator << (const float val)
{
    if (m_data)
    {
        int n = snprintf(m_data + m_size, (m_maxSize - m_size), "%0.2f", val);
        if (n > 0)
        {
            m_size += n;
            if (m_size >= m_maxSize)
            {
                m_size = m_maxSize;
                m_data[m_size - 1] = 0;
            }
        }
    }

    return *this;
}

void StrStream::reset()
{
    if (m_size > 0)
    {
        memset(m_data, 0, sizeof(char) * m_size);
        m_size = 0;
    }
}

const char* StrStream::str() const
{
    if (m_data)
    {
        return m_data;
    }
    else
    {
        return "";
    }
}

bool StrStream::empty() const
{
    return (m_size == 0);
}

uint32_t StrStream::size() const
{
    return m_size;
}
}
