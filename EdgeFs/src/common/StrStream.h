#pragma once
#include <stdint.h>
#include <string>

namespace apd_vp2p
{
class StrStream
{
public:
    StrStream();
    explicit StrStream(uint32_t maxSize);
    ~StrStream();

public:
    StrStream& operator << (uint8_t val);
    StrStream& operator << (uint16_t val);
    StrStream& operator << (uint32_t val);
    StrStream& operator << (uint64_t val);
    StrStream& operator << (int64_t val);
#ifdef __APPLE__
    StrStream& operator << (size_t val); // for mac
#endif
    StrStream& operator << (int val);
    StrStream& operator << (const std::string& str);
    StrStream& operator << (const char* val);
    StrStream& operator << (const float val);

public:
    void reset();
    const char* str() const;
    bool empty() const;
    uint32_t size() const;

private:
    char* m_data;
    uint32_t m_size;
    uint32_t m_maxSize;
};
}
