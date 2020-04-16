#include "Bitmap.h"
#include "common/common.h"

Bitmap::Bitmap()
: m_ptr(NULL)
, m_bitmapSize(0)
, m_idxNum(0)
{
}

Bitmap::~Bitmap()
{
}

void Bitmap::initBitmap(void* ptr, uint32_t bitmapSize, uint32_t idxNum)
{
    m_idxNum = idxNum;
    m_bitmapSize = bitmapSize;
    m_ptr = (uint8_t*)ptr;
}

bool Bitmap::generateIdleChunkids(std::vector<uint32_t>& idleChunkids, uint32_t needChunkNum)
{
    if (0 == needChunkNum)
    {
        return true;        
    }

    static std::random_device r;
    static std::default_random_engine e(r());
    static std::uniform_int_distribution<uint32_t> dist(0, -1);

    uint32_t tmp = dist(e) % m_idxNum;

    auto func = [&](uint32_t start, uint32_t end) -> bool {
        for (uint32_t i = start; i < end; i ++)
        {
            // TODO 此处获取的chunkid仍然是连续的
            if (isHave(i))
            {
                continue;
            }
            idleChunkids.push_back(i);
            if (idleChunkids.size() >= needChunkNum)
            {
                return true;
            }
        }
        return false;
    };

    if (func(tmp, m_idxNum) || func(0, tmp))
    {
        return true;
    }

    idleChunkids.clear();
    return false;
}

bool Bitmap::isHave( uint32_t idx )
{
    if (idx >= m_idxNum)
    {
        return false;
    }

    uint8_t* dest = m_ptr + idx / 8;
    uint8_t mask = 1 << idx % 8;

    return ( (*dest) & mask ) ? true : false;
}

bool Bitmap::insert( uint32_t idx )
{
    if (idx >= m_idxNum)
    {
        return false;
    }

    uint8_t* dest = m_ptr + idx / 8;
    uint8_t mask = 1 << idx % 8;

    *dest |= mask;

    return true;
}

bool Bitmap::insert(const std::vector<uint32_t>& idxs)
{
    for (auto it = idxs.begin(); it != idxs.end(); ++it)
    {
        insert(*it);
    }
    return true;
}