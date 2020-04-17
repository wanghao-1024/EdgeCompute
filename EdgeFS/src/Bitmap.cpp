#include "Bitmap.h"
#include "common/common.h"

Bitmap::Bitmap()
: m_ptr(NULL)
, m_bitmapSize(0)
, m_totalChunkNum(0)
, m_writeChunkNum(0)
{
}

Bitmap::~Bitmap()
{
}

void Bitmap::initBitmap(void* ptr, uint32_t bitmapSize, uint32_t idxNum)
{
    m_totalChunkNum = idxNum;
    m_writeChunkNum = 0;
    m_bitmapSize = bitmapSize;
    m_ptr = (uint8_t*)ptr;
}

bool Bitmap::generateIdleChunkids(const MetaInfo* pTailMtInfo, uint32_t tailChunkid,
    std::vector<uint32_t>& idleChunkids, uint32_t needChunkNum)
{
    if (0 == needChunkNum)
    {
        return true;        
    }

    if (!pTailMtInfo->m_isUsed)
    {
        idleChunkids.push_back(tailChunkid);
    }

    if (idleChunkids.size() >= needChunkNum)
    {
        return true;
    }

    if (m_writeChunkNum * 2 >= m_totalChunkNum)
    {
        // 磁盘使用率大于50%，则使用稳定的查找算法
        return generateIdleChunkidsForSteady(idleChunkids, needChunkNum, tailChunkid);
    }
    // 磁盘使用率小于50%，则使用随机的查找算法
    return generateIdleChunkidsForFast(idleChunkids, needChunkNum, tailChunkid);
}

bool Bitmap::generateIdleChunkidsForFast(std::vector<uint32_t>& idleChunkids,
    uint32_t needChunkNum, uint32_t excludeChunkid)
{
    // 系统刚运营时磁盘比较空，选择随机分配chunk的方法
    static std::random_device r;
    static std::default_random_engine e(r());
    static std::uniform_int_distribution<uint32_t> dist(0, -1);
    uint32_t cnt = 0;

    while (1)
    {
        cnt += 1;
        if (cnt >= 10 * needChunkNum)
        {
            // 查找了10倍都未满足说明磁盘已经比较满了，则顺序查找
            break;
        }

        uint32_t chunkid = dist(e) % m_totalChunkNum;

        if (isHave(chunkid) ||
            chunkid == excludeChunkid)
        {
            continue;
        }
        idleChunkids.push_back(chunkid);
        if (idleChunkids.size() >= needChunkNum)
        {
            return true;
        }
    }

    for (size_t chunkid = 0; chunkid < m_totalChunkNum; chunkid++)
    {
        if (isHave(chunkid) ||
            chunkid == excludeChunkid)
        {
            continue;
        }
        idleChunkids.push_back(chunkid);
        if (idleChunkids.size() >= needChunkNum)
        {
            return true;
        }
    }

    idleChunkids.clear();
    return false;
}
bool Bitmap::generateIdleChunkidsForSteady(std::vector<uint32_t>& idleChunkids,
    uint32_t needChunkNum, uint32_t excludeChunkid)
{
    // 系统刚运营时磁盘比较满时，则直接循环
    // 因为随机算法的耗时时不稳定的

    for (size_t chunkid = 0; chunkid < m_totalChunkNum; chunkid++)
    {
        if (isHave(chunkid) ||
            chunkid == excludeChunkid)
        {
            continue;
        }
        idleChunkids.push_back(chunkid);
        if (idleChunkids.size() >= needChunkNum)
        {
            return true;
        }
    }

    idleChunkids.clear();
    return false;
}

bool Bitmap::isHave( uint32_t idx )
{
    if (idx >= m_totalChunkNum)
    {
        return false;
    }
    
    uint8_t* dest = m_ptr + idx / 8;
    uint8_t mask = 1 << idx % 8;

    return ( (*dest) & mask ) ? true : false;
}

bool Bitmap::insert( uint32_t idx )
{
    if (idx >= m_totalChunkNum)
    {
        return false;
    }

    m_writeChunkNum += 1;

    uint8_t* dest = m_ptr + idx / 8;
    uint8_t mask = 1 << idx % 8;

    *dest |= mask;

    return true;
}

void Bitmap::insert(const std::vector<uint32_t>& idxs)
{
    for (auto it = idxs.begin(); it != idxs.end(); ++it)
    {
        insert(*it);
    }
}