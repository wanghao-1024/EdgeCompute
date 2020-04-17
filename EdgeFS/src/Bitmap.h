#pragma once

#include "common/SystemHead.h"
#include "EdgeFSProtocol.h"

class Bitmap
{
public:
    Bitmap();
    ~Bitmap();

public:
    void initBitmap(void* ptr, uint32_t bitmapSize, uint32_t idxNum);

    bool generateIdleChunkids(const MetaInfo* pTailMtInfo, uint32_t tailChunkid,
        std::vector<uint32_t>& idleChunkids, uint32_t needChunkNum);

    bool isHave(uint32_t idx);

    bool insert(uint32_t idx);

    void insert(const std::vector<uint32_t>& idxs);

private:
    bool generateIdleChunkidsForFast(std::vector<uint32_t>& idleChunkids,
        uint32_t needChunkNum, uint32_t excludeChunkid);
    bool generateIdleChunkidsForSteady(std::vector<uint32_t>& idleChunkids,
        uint32_t needChunkNum, uint32_t excludeChunkid);

public:
    void* getPtr()
    {
        return (void*)m_ptr;
    }

public:
    uint8_t*    m_ptr;
    uint32_t    m_bitmapSize;
    uint32_t    m_totalChunkNum;
    uint32_t    m_writeChunkNum;
};
