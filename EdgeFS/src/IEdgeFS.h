#pragma once

#include <stdint.h>
#include <string>

typedef struct SystemInfo_
{
    uint64_t        m_diskCapacity;
    std::string     m_diskRootDir;
    uint64_t        m_edgeFSUsableMemory;
    
    SystemInfo_()
    : m_diskCapacity(0)
    , m_edgeFSUsableMemory(0)
    {}
} SystemInfo;

class IEdgeFS
{
public:

    virtual bool initFS(const SystemInfo& info) = 0;

    virtual void unitFS() = 0;
    
    /*
    @return : -1表示读取失败，否则返回写入成功的字节数
    // TODO 需要定义详细读取错误的错误码
    */
    virtual int64_t read(const std::string& fileName, char* buff, uint32_t len, uint64_t offset) = 0;

    /*
    @return : -1表示写入失败，否则返回写入成功的字节数
    // TODO 需要定义详细写入错误的错误码
    */
    virtual int64_t write(const std::string& fileName, const char* buff, uint32_t len) = 0;
};

IEdgeFS* CreateEdgeFS();

void DestroyPcdnSdk(IEdgeFS* fs);