#pragma once
#include "common/common.h"
#include "DataMgr.h"
#include "IndexMgr.h"
#include "Bitmap.h"
#include "IEdgeFS.h"
#include "EdgeFSConst.h"
#include "EdgeFSProtocol.h"

class EdgeFS : public IEdgeFS
{
public:
    static void create();
    static void release();
    static EdgeFS* instance();

public:
    EdgeFS();
    virtual ~EdgeFS();

public:
    virtual bool initFS(const SystemInfo& info);
    virtual void unitFS();
    virtual int64_t read(const std::string& fileName, char* buff, uint32_t len, uint64_t offset);
    virtual int64_t write(const std::string& fileName, const char* buff, uint32_t len);

private:
    // init
    bool initFSCheckParam(const SystemInfo& info);
    bool initFSCalcVariable(const SystemInfo& info, uint32_t& chunkNum, uint32_t& chunkSize, uint64_t& diskSize,
        uint32_t& bitmapSize, uint64_t& mmapSize);
    bool initFSCalcPointerAddr(bool isExistsIdxFile, uint32_t chunkNum, uint32_t chunkSize, uint64_t diskSize, uint32_t bitmapSize,
        uint64_t mmapSize);
    bool initFSCalcPointerAddrForCreateIdxFile(uint32_t chunkNum, uint32_t chunkSize, uint64_t diskSize, uint32_t bitmapSize,
        uint64_t mmapSize);
    bool initFSCalcPointerAddrForReloadIdxFile(uint32_t chunkNum, uint32_t chunkSize, uint64_t diskSize, uint32_t bitmapSize,
        uint64_t mmapSize);

    // write
    void calcWriteVariable(const MetaInfo* pTailMtInfo, uint32_t writeLen, uint32_t& firstWriteLen,
        uint32_t& needChunkNum, uint32_t& lastChunkWriteLen);
    MetaInfo* findTailMetaInfo(const MetaInfo* pHeadMtInfo, const char* sha1Val);

    // read
    void generateReadChunkids(const MetaInfo* pHeadMtInfo, const char* sha1Val,
        std::vector<uint32_t>& writeChunkids, uint64_t& writeTotalLen, uint32_t& lastChunkidWriteLen);
    void calcReadVariable(const std::vector<uint32_t>& writeChunkids, uint32_t lastChunkWriteLen,
        uint32_t readLen, uint64_t offset, std::map<uint64_t, uint32_t>& readInfo);

    // common
    uint32_t calcChunkid(const MetaInfo* pMtInfo);
    MetaInfo* calcMetaInfoPtr(uint32_t chunkid);
    uint64_t calcOffset(uint32_t chunkid);
    uint32_t generateHashKey(const char* sha1Val);

private:
    void printAllMetaInfo();

private:
    static EdgeFS*          m_pInstance;
    // mmap映射在文件中
    EdgeFSHead*             m_pFSHead;
    Bitmap*                 m_pBitMap;
    MetaInfo*               m_pMetaPool;

    DataMgr*                m_pDataMgr;
    IndexMgr*               m_pIndexMgr;
};