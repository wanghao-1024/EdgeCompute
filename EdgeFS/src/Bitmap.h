#pragma once

#include "common/SystemHead.h"

class Bitmap
{
public:
    Bitmap();
    ~Bitmap();

public:
    void initBitmap(void* ptr, uint32_t bitmapSize, uint32_t idxNum);

    bool generateIdleChunkids(std::vector<uint32_t>& idleChunkids, uint32_t needChunkNum);

    bool isHave(uint32_t idx);

    bool insert(uint32_t idx);

    bool insert(const std::vector<uint32_t>& idxs);

public:
    void* getPtr()
    {
        return (void*)m_ptr;
    }

public:
    uint8_t*    m_ptr;
    uint32_t    m_bitmapSize;
    uint32_t    m_idxNum;
};
