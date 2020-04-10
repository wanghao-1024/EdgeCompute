#pragma once

#include "common/common.h"
#include "EdgeFSConst.h"

#pragma pack(1)

typedef struct EdgeFSHead_
{
    char            m_magic[12];
    uint64_t        m_coverableDiskSize;    // 有限的内存可以覆盖的磁盘大小
    uint64_t        m_usableMemory;         // fs需要使用的内存
    uint32_t        m_chunkSize;            // 每个chunk块的大小
    uint32_t        m_chunkNum;             // chunk块的个数
    uint32_t        m_bitmapSize;           // bitmap占用的字节数

    EdgeFSHead_()
    : m_coverableDiskSize(0)
    , m_usableMemory(0)
    , m_chunkSize(0)
    , m_chunkNum(0)
    , m_bitmapSize(0)
    {
        memset(m_magic, 0, sizeof(m_magic));
    }
} EdgeFSHead;

typedef struct MetaData_
{
    /* 暂时不设置时间，因为边缘设备用户会修改时间
    uint32_t    m_createStamp;
    uint32_t    m_lastUpdateStamp;      // 读写的时候都更新
    */

    char        m_sha1[SHA_DIGEST_LENGTH];
    uint64_t    m_fileSize;     // TODO暂时未使用
    uint32_t    m_crc32;        // TODO暂时未使用

    MetaData_()
    : m_fileSize(0)
    , m_crc32(0)
    {
        memset(m_sha1, 0, sizeof(m_sha1));
    }
} MetaData;

// 最大不超过8KB
typedef struct ExtendArea_
{
    /*
    暂时未使用
    */
   ExtendArea_()
   {}
} ExtendArea;

typedef struct MetaInfo_
{
    bool            m_isUsed;   // 是否被占用

    // 整个文件的信息，存储该文件的每个chunk块应该一致
    MetaData        m_metaData;

    // 当前chunk块的信息
    uint32_t                m_idleLen;     // chunk空闲的长度

    // 下一个chunk块的信息
    uint32_t                m_nextChunkid;          // 下一个chunkid

    // 扩展信息    
    ExtendArea      m_extendArea;

    MetaInfo_()
    : m_isUsed(false)
    , m_idleLen(0)
    , m_nextChunkid(kInvalidChunkid)
    {}

    std::string print()
    {
        std::stringstream ss;

        ss << "used " << m_isUsed
            //<< " sha1 " << m_metaData.m_sha1
            //<< " fileSize " << m_metaData.m_fileSize
            << " idleLen " << m_idleLen
            << " nextChunkid " << (int32_t)m_nextChunkid;

        return ss.str();
    }
} MetaInfo;

#pragma pack()

