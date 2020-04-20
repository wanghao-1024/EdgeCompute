#include "EdgeFS.h"
#include "EdgeFSConst.h"
#include "EdgeFSDebugSwitch.h"
#include "./common/common.h"

EdgeFS* EdgeFS::m_pInstance = NULL;

IEdgeFS* CreateEdgeFS()
{
    EdgeFS::create();
    return EdgeFS::instance();
}

void DestroyPcdnSdk(IEdgeFS* fs)
{
    EdgeFS::release();
}

void EdgeFS::create()
{
    if (NULL == m_pInstance)
    {
        m_pInstance = new EdgeFS();
    }
}
void  EdgeFS::release()
{
    SAFE_DELETE(m_pInstance);
}

EdgeFS* EdgeFS::instance()
{
    return m_pInstance;
}

EdgeFS::EdgeFS()
: m_nextTaskid(1)
, m_currTaskid(-1)
{
    m_pFSHead = NULL;
    m_pMetaPool = NULL;

    m_pDataMgr = new DataMgr();
    m_pIndexMgr = new IndexMgr();
    m_pBitMap = new Bitmap();
}
EdgeFS::~EdgeFS()
{
    SAFE_DELETE(m_pBitMap);
    SAFE_DELETE(m_pIndexMgr);
    SAFE_DELETE(m_pDataMgr);
}

bool EdgeFS::initFS(const SystemInfo& info)
{
    if (!initFSImpl(info))
    {
        initFSFailRollBack();
        return false;
    }
    return true;
}

void EdgeFS::initFSFailRollBack()
{
    m_pDataMgr->unlink();
    m_pIndexMgr->unlink();
}

bool EdgeFS::initFSImpl(const SystemInfo& info)
{
    AsyncLogging::create();
    AsyncLogging::instance()->init(info.m_diskRootDir + "/" + kLogFileName);

    linfo("========================");

    lnotice("initFs, systemInfo disk %" PRIu64 " rootdir %s memory %" PRIu64, info.m_diskCapacity,
        info.m_diskRootDir.c_str(), info.m_edgeFSUsableMemory);

    // 入参数检查
    if (!initFSCheckParam(info))
    {
        return false;
    }

    // 根据收入的内存大小和磁盘大小，计算chunk个数，chunk大小，需要映射的内存
    uint32_t chunkNum = 0, chunkSize = 0, bitmapSize = 0;
    uint64_t diskSize = 0, mmapSize = 0;
    if (!initFSCalcVariable(info, chunkNum, chunkSize, diskSize, bitmapSize, mmapSize))
    {
        return false;
    }

    // 初始化数据文件和index文件
    // TODO macos上面已经支持大文件了, linux系统后续需要用mmap64, ftruncate64等
    bool isExistIdxFile = false;
    if (!m_pDataMgr->initDataMgr(info.m_diskRootDir, diskSize) ||
        !m_pIndexMgr->initIndexMgr(info.m_diskRootDir, mmapSize, isExistIdxFile))
    {
        return false;
    }

    if (!initFSCalcPointerAddr(isExistIdxFile, chunkNum, chunkSize, diskSize, bitmapSize, mmapSize))
    {
        return false;
    }
    return true;
}

bool EdgeFS::initFSCheckParam(const SystemInfo& info)
{
    // 内存检查，最少1个meta占用的内存
    uint64_t minMemory = sizeof(EdgeFSHead) + 1 + sizeof(MetaInfo);
    if (minMemory >= info.m_edgeFSUsableMemory)
    {
        lfatal("initFS failed, out of memory, minimum %" PRIu64 " memory", minMemory);
        return false;
    }
    return true;
}

bool EdgeFS::initFSCalcVariable(const SystemInfo& info, uint32_t& chunkNum, uint32_t& chunkSize,
    uint64_t& diskSize, uint32_t& bitmapSize, uint64_t& mmapSize)
{
    chunkNum = DIV_ROUND_DOWN(info.m_edgeFSUsableMemory - sizeof(EdgeFSHead), sizeof(MetaInfo));
    chunkSize = DIV_ROUND_DOWN(info.m_diskCapacity, chunkNum);
    chunkSize = ALIGN_DOWN(chunkSize, kDiskRWAlignSize);
    Utils::limit<uint32_t>(chunkSize, kMinChunkSize, kMaxChunkSize);
    chunkNum = DIV_ROUND_DOWN(info.m_diskCapacity, chunkSize);
    bitmapSize = DIV_ROUND_UP(chunkNum, 8);
    diskSize = (uint64_t)chunkNum * (uint64_t)chunkSize;
    mmapSize = sizeof(EdgeFSHead) + bitmapSize + (uint64_t)chunkNum * sizeof(MetaInfo);

    if (0 == chunkNum ||
        0 == chunkSize ||
        0 == bitmapSize ||
        0 == diskSize ||
        0 == mmapSize ||
        diskSize > info.m_diskCapacity ||
        mmapSize > info.m_edgeFSUsableMemory)
    {
        lfatal("initFS failed, calc variable failed, chunkNum %u chunkSize %u bitmapSize %u"
            " diskSize %" PRIu64 " mmapSize %" PRIu64,
            chunkNum, chunkSize, bitmapSize, diskSize, mmapSize);
        lfatal("initFS failed, calc variable failed, diskSize too large: %d diskSize %" PRIu64
            " info.m_diskCapacity %" PRIu64,
            diskSize >= info.m_diskCapacity, diskSize, info.m_diskCapacity);
        lfatal("initFS failed, calc variable failed, mmapSize too large: %d mmapSize %" PRIu64
            " info.m_edgeFSUsableMemory %" PRIu64,
            mmapSize >= info.m_edgeFSUsableMemory, mmapSize, info.m_edgeFSUsableMemory);
        return false;
    }

    linfo("chunkNum %d chunkSize %u bitmapSize %u mmapSize %" PRIu64 " diskSize %" PRIu64,
        chunkNum, chunkSize, bitmapSize, mmapSize, diskSize);
    linfo("EdgeFSHead size %zu MetaInfo size %zu", sizeof(EdgeFSHead), sizeof(MetaInfo));

    return true;
}

bool EdgeFS::initFSCalcPointerAddr(bool isExistsIdxFile, uint32_t chunkNum, uint32_t chunkSize,
    uint64_t diskSize, uint32_t bitmapSize, uint64_t mmapSize)
{
    if (isExistsIdxFile)
    {
        return initFSCalcPointerAddrForReloadIdxFile(chunkNum, chunkSize, diskSize, bitmapSize,
            mmapSize);
    }
    return initFSCalcPointerAddrForCreateIdxFile(chunkNum, chunkSize, diskSize, bitmapSize,
        mmapSize);
}

bool EdgeFS::initFSCalcPointerAddrForCreateIdxFile(uint32_t chunkNum, uint32_t chunkSize,
    uint64_t diskSize, uint32_t bitmapSize, uint64_t mmapSize)
{
    int fd = m_pIndexMgr->getfd();
    char *ptr = (char *)mmap(0, mmapSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (MAP_FAILED == ptr)
    {
        lfatal("initFS failed, mmap failed, mmapSize %" PRIu64 " fd %d err %s", mmapSize, fd,
            strerror(errno));
        return false;
    }
    memset(ptr, 0, mmapSize);

    // 赋值指针
    m_pFSHead = (EdgeFSHead*)ptr;
    m_pBitMap->initBitmap((char*)m_pFSHead + sizeof(EdgeFSHead), bitmapSize, chunkNum);
    m_pMetaPool = (MetaInfo*)((char*)m_pBitMap->getPtr() + bitmapSize);

    linfo("start %p fsHead %p bitmap %p metapool %p", ptr, m_pFSHead, m_pBitMap->getPtr(),
        m_pMetaPool);
    
    // 赋值FS头部信息
    memcpy(m_pFSHead->m_magic, kEdgeFSMagic.c_str(), kEdgeFSMagic.size());
    m_pFSHead->m_usableMemory = mmapSize;
    m_pFSHead->m_coverableDiskSize = diskSize;
    m_pFSHead->m_chunkNum = chunkNum;
    m_pFSHead->m_chunkSize = chunkSize;
    m_pFSHead->m_bitmapSize = bitmapSize;

    linfo("index file EdgeFSHead, magic %s memory %" PRIu64 " diskSize %" PRIu64 " chunkNum %u"
        " chunkSize %u bitmapSize %u",
        m_pFSHead->m_magic, m_pFSHead->m_usableMemory, m_pFSHead->m_coverableDiskSize,
        m_pFSHead->m_chunkNum, m_pFSHead->m_chunkSize, m_pFSHead->m_bitmapSize);

    return true;
}

bool EdgeFS::initFSCalcPointerAddrForReloadIdxFile(uint32_t chunkNum, uint32_t chunkSize,
    uint64_t diskSize, uint32_t bitmapSize, uint64_t mmapSize)
{
    int fd = m_pIndexMgr->getfd();
    if (-1 == fd)
    {
        lfatal("initFS failed, fd error");
        return false;
    }
    char *ptr = (char *)mmap(0, mmapSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (MAP_FAILED == ptr)
    {
        lfatal("initFS failed, mmap failed, mmapSize %" PRIu64 " fd %d err %s", mmapSize, fd,
            strerror(errno));
        return false;
    }

    m_pFSHead = (EdgeFSHead*)ptr;
    m_pBitMap->initBitmap((char*)m_pFSHead + sizeof(EdgeFSHead), bitmapSize, chunkNum);
    m_pMetaPool = (MetaInfo*)((char*)m_pBitMap->getPtr() + bitmapSize);

    linfo("start %p fsHead %p bitmap %p metapool %p", ptr, m_pFSHead, m_pBitMap->getPtr(),
        m_pMetaPool);

    // 对index文件中的FS头部信息做校验
    if (0 != memcmp(m_pFSHead->m_magic, kEdgeFSMagic.c_str(), kEdgeFSMagic.size()) ||
        m_pFSHead->m_usableMemory != mmapSize ||
        m_pFSHead->m_coverableDiskSize != diskSize ||
        m_pFSHead->m_chunkNum != chunkNum ||
        m_pFSHead->m_chunkSize != chunkSize ||
        m_pFSHead->m_bitmapSize != bitmapSize
        )
    {
        lfatal("initFS failed, index file EdgeFSHead error, magic %s %s memory %" PRIu64
            " %" PRIu64 " diskSize %" PRIu64 " %" PRIu64 " chunkNum %u %u chunkSize %u %u"
            " bitmapSize %u %u",
            m_pFSHead->m_magic, kEdgeFSMagic.c_str(), m_pFSHead->m_usableMemory, mmapSize,
            m_pFSHead->m_coverableDiskSize, diskSize, m_pFSHead->m_chunkNum, chunkNum,
            m_pFSHead->m_chunkSize, chunkSize, m_pFSHead->m_bitmapSize, bitmapSize);
        return false;
    }

    linfo("index file EdgeFSHead, magic %s memory %" PRIu64 " diskSize %" PRIu64 "chunkNum %u"
        " chunkSize %u bitmapSize %u",
        m_pFSHead->m_magic, m_pFSHead->m_usableMemory, m_pFSHead->m_coverableDiskSize,
        m_pFSHead->m_chunkNum, m_pFSHead->m_chunkSize, m_pFSHead->m_bitmapSize);

    printAllMetaInfo();

    return true;
}

void EdgeFS::unitFS()
{
    AsyncLogging::release();
}

void EdgeFS::calcWriteVariable(const MetaInfo* pTailMtInfo, uint32_t writeLen, uint32_t& firstWriteLen,
    uint32_t& needChunkNum, uint32_t& lastChunkWriteLen)
{
    if (NULL != pTailMtInfo)
    {
        if (!pTailMtInfo->m_isUsed)
        {
            // chunk块没有被使用
            // fistWriteLen是为了先将最后一个未满的chunk填满
            firstWriteLen = 0;
        }
        else
        {
            // 当chunk块写满时，idleLen为0，则firstWriteLen也是0
            firstWriteLen = writeLen >= pTailMtInfo->m_idleLen ?
                pTailMtInfo->m_idleLen : writeLen;
        }
    }
    else
    {
        // 首次写入该文件
        firstWriteLen = 0;
    }
    
    uint64_t remainLen = writeLen - firstWriteLen;
    needChunkNum = DIV_ROUND_UP(remainLen, m_pFSHead->m_chunkSize);
    lastChunkWriteLen = (0 == remainLen % m_pFSHead->m_chunkSize) ?
        m_pFSHead->m_chunkSize : remainLen % m_pFSHead->m_chunkSize;
        
    linfo("taskid %u pTailMtInfo %p writeLen %u firstWriteLen %u needChunkNum %u"
        " lastChunkWriteLen %u",
        m_currTaskid, pTailMtInfo, writeLen, firstWriteLen, needChunkNum, lastChunkWriteLen);
}

MetaInfo* EdgeFS::findChunkTailMetaInfo(const MetaInfo* pHeadMtInfo)
{
    if (!pHeadMtInfo->m_isUsed)
    {
        return const_cast<MetaInfo*>(pHeadMtInfo);
    }

    const MetaInfo* pNextMtInfo = pHeadMtInfo;
    while (1)
    {
        if (kInvalidChunkid == pNextMtInfo->m_nextChunkid)
        {
            return const_cast<MetaInfo*>(pNextMtInfo);
        }
        pNextMtInfo = calcMetaInfoPtr(pNextMtInfo->m_nextChunkid);
    }
    assert(0);
    return NULL;
}

MetaInfo* EdgeFS::findFileTailMetaInfo(const MetaInfo* pHeadMtInfo, const char* sha1Val)
{
    if (!pHeadMtInfo->m_isUsed)
    {
        return const_cast<MetaInfo*>(pHeadMtInfo);
    }

    const MetaInfo* pTailMtInfo = NULL;
    const MetaInfo* pNextMtInfo = pHeadMtInfo;

    while (1)
    {
        if (0 == memcmp(pNextMtInfo->m_metaData.m_sha1, sha1Val, sizeof(pNextMtInfo->m_metaData.m_sha1)))
        {
            pTailMtInfo = pNextMtInfo;
        }
        if (kInvalidChunkid == pNextMtInfo->m_nextChunkid)
        {
            break;
        }
        pNextMtInfo = calcMetaInfoPtr(pNextMtInfo->m_nextChunkid);
    }
    return const_cast<MetaInfo*>(pTailMtInfo);
}

uint32_t EdgeFS::calcChunkid(const MetaInfo* pMtInfo)
{
    if (NULL == pMtInfo)
    {
        return kInvalidChunkid;
    }
    return ((char*)pMtInfo - (char*)m_pMetaPool) / (uint64_t)sizeof(MetaInfo);
}

MetaInfo* EdgeFS::calcMetaInfoPtr(uint32_t chunkid)
{
    return (MetaInfo*)((char*)m_pMetaPool + (uint64_t)chunkid * (uint64_t)sizeof(MetaInfo));
}

uint64_t EdgeFS::calcOffset(uint32_t chunkid)
{
    return (uint64_t)chunkid * (uint64_t)(m_pFSHead->m_chunkSize);
}

uint32_t EdgeFS::generateHashKey(const char* sha1Val)
{
    if (kHashCollisions)
    {
        return 101;
    
    }
    return (*(uint32_t*)sha1Val) % m_pFSHead->m_chunkNum;
}

int64_t EdgeFS::write(const std::string& fileName, const char* buff, uint32_t len)
{
    if (NULL == buff)
    {
        return -1;
    }
    m_currTaskid = generateTaskid();

    lnotice("taskid %u fileName %s len %u", m_currTaskid, fileName.c_str(), len);

    char sha1Val[SHA_DIGEST_LENGTH] = { '\0' };
    ShaHelper::calcShaToHex(fileName, sha1Val);

    uint32_t hashKey = generateHashKey(sha1Val);
    MetaInfo* pHeadMtInfo = calcMetaInfoPtr(hashKey);
    MetaInfo* pCurrFileTailMtInfo = findFileTailMetaInfo(pHeadMtInfo, sha1Val);
    MetaInfo* pChunkTailMtInfo = findChunkTailMetaInfo(pHeadMtInfo);

    linfo("taskid %u hashKey %u pHeadMtInfo %p %d pCurrFileTailMtInfo %p %d pChunkTailMtInfo %p %d",
        m_currTaskid, hashKey, pHeadMtInfo, calcChunkid(pHeadMtInfo), pCurrFileTailMtInfo,
        calcChunkid(pCurrFileTailMtInfo), pChunkTailMtInfo, calcChunkid(pChunkTailMtInfo));

    uint32_t firstWriteLen = 0;
    uint32_t needChunkNum = 0;
    std::vector<uint32_t> idleChunkids;
    uint32_t lastChunkWriteLen = 0;
    
    calcWriteVariable(pCurrFileTailMtInfo, len, firstWriteLen, needChunkNum, lastChunkWriteLen);

    if (0 != needChunkNum)
    {
        if (!m_pBitMap->generateIdleChunkids(pCurrFileTailMtInfo, calcChunkid(pCurrFileTailMtInfo),
            idleChunkids, needChunkNum)
            || idleChunkids.empty())
        {
            lwarn("taskid %u no idle chunk", m_currTaskid);
            return -1;
        }
    }
    linfo("taskid %u idleChunkSize %zu", m_currTaskid, idleChunkids.size());

    uint64_t remainLen = len;
    uint64_t offset = 0;
    uint32_t chunkid = 0;
    uint32_t realWriteLen = 0;
    uint32_t chunkSize = m_pFSHead->m_chunkSize;

    do
    {
        if (0 != firstWriteLen)
        {
            chunkid = calcChunkid(pCurrFileTailMtInfo);
            offset = (uint64_t)chunkid * (uint64_t)chunkSize;
            // 要注意pTailMtInfo使用的是共享内存的地址，成员变量默认都是0
            if (pCurrFileTailMtInfo->m_isUsed)
            {
                offset += chunkSize - pCurrFileTailMtInfo->m_idleLen;
            }

            linfo("taskid %u first write, firstWriteLen %u chunkid %u offset %" PRIu64,
                m_currTaskid, firstWriteLen, chunkid, offset);

            if (!m_pDataMgr->write(buff+realWriteLen, firstWriteLen, offset))
            {
                lerror("taskid %u write failed, writeLen %u offset %" PRIu64, m_currTaskid, len,
                    offset);
                break;
            }
            realWriteLen += firstWriteLen;
            remainLen -= firstWriteLen;

            if (pCurrFileTailMtInfo->m_isUsed)
            {
                pCurrFileTailMtInfo->m_idleLen -= firstWriteLen;
            }
            else
            {
                m_pBitMap->insert(chunkid);
                pCurrFileTailMtInfo->m_isUsed = true;
                memcpy(pCurrFileTailMtInfo->m_metaData.m_sha1, sha1Val,
                    sizeof(pCurrFileTailMtInfo->m_metaData.m_sha1));
                pCurrFileTailMtInfo->m_idleLen = chunkSize - firstWriteLen;
                pCurrFileTailMtInfo->m_nextChunkid = kInvalidChunkid;
            }
            if (0 == pCurrFileTailMtInfo->m_idleLen)
            {
                // 当前chunk已经写满了，则赋值最后一个chunk的nextChunkid
                // 请注意这里不是赋值pCurrFileTailMtInfo
                // 请详细看ReadMe.md的元数据区的图
                pChunkTailMtInfo->m_nextChunkid =
                    idleChunkids.empty() ? kInvalidChunkid : idleChunkids[0];
            }

            linfo("taskid %u chunkid %u offset %" PRIu64 " tailMetaInfo %s", m_currTaskid, chunkid,
                offset, pCurrFileTailMtInfo->print().c_str());
        }
        else
        {
            // firstWriteLen为0，则说明filename的最后一个chunk块写满了，则赋值最后一个chunk块的nextChunkid
            if (!idleChunkids.empty())
            {
                pChunkTailMtInfo->m_nextChunkid = idleChunkids[0];
            }
        }

        if (0 != needChunkNum)
        {
            for (uint32_t i = 0; i < needChunkNum; i++)
            {
                uint32_t writeLen = remainLen >=  chunkSize ? chunkSize : remainLen;
                chunkid = idleChunkids[i];
                offset = (uint64_t)chunkid * (uint64_t)chunkSize;
                MetaInfo* pCurrMtInfo = calcMetaInfoPtr(chunkid);

                linfo("taskid %u chunkid %u offset %" PRIu64 " writeLen %u buffOffset %u",
                    m_currTaskid, chunkid, offset, writeLen, realWriteLen);

                if (!m_pDataMgr->write(buff+realWriteLen, writeLen, offset))
                {
                    lerror("taskid %u write failed, writeLen %u offset %" PRIu64, m_currTaskid,
                        len, offset);
                    break;
                }
                remainLen -= writeLen;
                realWriteLen += writeLen;

                // 写入成功更新bitmap、metainfo
                m_pBitMap->insert(chunkid);
                pCurrMtInfo->m_isUsed = true;
                memcpy(pCurrMtInfo->m_metaData.m_sha1, sha1Val,
                    sizeof(pCurrMtInfo->m_metaData.m_sha1));
                pCurrMtInfo->m_idleLen = chunkSize - firstWriteLen;

                if (i + 1 == needChunkNum)
                {
                    // 最后一个chunk
                    pCurrMtInfo->m_nextChunkid = kInvalidChunkid;
                    pCurrMtInfo->m_idleLen = chunkSize - lastChunkWriteLen;
                }
                else
                {
                    pCurrMtInfo->m_nextChunkid = idleChunkids[i+1];
                    pCurrMtInfo->m_idleLen = 0;
                }
                linfo("taskid %u metaInfo %s", m_currTaskid, pCurrMtInfo->print().c_str());
            }
        }
    } while (0);

    printAllMetaInfo();

    return realWriteLen;
}

int64_t EdgeFS::read(const std::string& fileName, char* buff, uint32_t len, uint64_t offset)
{
    if (NULL == buff)
    {
        return -1;
    }
    m_currTaskid = generateTaskid();

    lnotice("taskid %u fileName %s len %u offset %" PRIu64, m_currTaskid, fileName.c_str(), len,
        offset);

    char sha1Val[SHA_DIGEST_LENGTH] = { '\0' };
    ShaHelper::calcShaToHex(fileName, sha1Val);

    uint32_t hashKey = generateHashKey(sha1Val);
    MetaInfo* pHeadMtInfo = calcMetaInfoPtr(hashKey);
    std::deque<uint32_t> writeChunkids;  // 这里deque要好于vector，因为vector扩容是会发生拷贝
    uint32_t lastChunkWriteLen = 0;
    uint64_t writeTotalLen = 0;

    generateReadChunkids(pHeadMtInfo, sha1Val, writeChunkids, writeTotalLen, lastChunkWriteLen);

    if (writeChunkids.empty())
    {
        lwarn("taskid %u not found file, fileName %s", m_currTaskid, fileName.c_str());
        return -1;
    }
    if (offset > writeTotalLen)
    {
        lwarn("taskid %u offset too large, offset %" PRIu64 " writeTotalLen %" PRIu64, m_currTaskid,
            offset, writeTotalLen);
        return -1;
    }

    linfo("taskid %u writeChunkNum %zu writeTotalLen %" PRIu64 " lastChunkWriteLen %u",
        m_currTaskid, writeChunkids.size(), writeTotalLen, lastChunkWriteLen);
    linfo("taskid %u write chunkids : ", m_currTaskid);
    for (uint32_t i = 0; i < writeChunkids.size(); i++)
    {
        ldebug("taskid %u chunkid %d ", m_currTaskid, writeChunkids[i]);
    }

    // calcReadVariable和generateReadChunkids合并可以优化性能和内存占用
    // 当文件比较大时，writeChunkids可能会内存消耗
    std::deque<std::pair<uint64_t, uint32_t>> readInfo;     // offset -> len
    calcReadVariable(writeChunkids, lastChunkWriteLen, len, offset, readInfo);

    for (auto it = readInfo.begin(); it != readInfo.end(); ++it)
    {
        linfo("taskid %u read chunkId %u offset %" PRIu64 " len %u", m_currTaskid,
            (uint32_t)(it->first/m_pFSHead->m_chunkSize), it->first, it->second);
    }

    uint32_t realReadLen = 0;
    for (auto it = readInfo.begin(); it != readInfo.end(); ++it)
    {
        if (!m_pDataMgr->read(buff+realReadLen, it->second, it->first))
        {
            lerror("taskid %u read failed, chunkid %u offset %" PRIu64 " readLen %u", m_currTaskid,
                (uint32_t)(it->first/m_pFSHead->m_chunkSize), it->first, it->second);
            break;
        }
        realReadLen += it->second;
    }
    return realReadLen;
}

void EdgeFS::generateReadChunkids(const MetaInfo* pHeadMtInfo, const char* sha1Val,
    std::deque<uint32_t>& writeChunkids, uint64_t& writeTotalLen, uint32_t& lastChunkidWriteLen)
{
    assert(NULL != pHeadMtInfo);
    assert(NULL != sha1Val);

    if (!pHeadMtInfo->m_isUsed)
    {
        writeChunkids.clear();
        writeTotalLen = 0;
        lastChunkidWriteLen = 0;
        return ;
    }

    const MetaInfo* tmp = pHeadMtInfo;
    while (true)
    {
        if (0 == memcmp(tmp->m_metaData.m_sha1, sha1Val, sizeof(tmp->m_metaData.m_sha1)))
        {
            writeChunkids.push_back(calcChunkid(tmp));
            lastChunkidWriteLen = m_pFSHead->m_chunkSize - tmp->m_idleLen;
            writeTotalLen += lastChunkidWriteLen;
        }
        if (kInvalidChunkid == tmp->m_nextChunkid)
        {
            break;
        }
        tmp = calcMetaInfoPtr(tmp->m_nextChunkid);
    }
}

void EdgeFS::calcReadVariable(const std::deque<uint32_t>& writeChunkids, uint32_t lastChunkWriteLen,
    uint32_t readLen, uint64_t offset, std::deque<std::pair<uint64_t, uint32_t>>& readInfo)
{
    const uint32_t chunkSize = m_pFSHead->m_chunkSize;
    const uint32_t firstReadIdx = DIV_ROUND_DOWN(offset, chunkSize);
    const uint32_t firstChunkSkipLen = offset % chunkSize;
    uint32_t firstChunkReadLen = 0;

    if (firstReadIdx + 1 == writeChunkids.size())
    {
        // 已经读取到最后一个chunk了
        firstChunkReadLen = std::min(lastChunkWriteLen - firstChunkSkipLen, readLen);
        readInfo.push_back(
            std::pair<uint64_t, uint32_t>(
                calcOffset(writeChunkids[firstReadIdx]) + firstChunkSkipLen,
                firstChunkReadLen));
        return ;
    }
    else
    {
        firstChunkReadLen = std::min(chunkSize - firstChunkSkipLen, readLen);
        readInfo.push_back(
            std::pair<uint64_t, uint32_t>(
                calcOffset(writeChunkids[firstReadIdx]) + firstChunkSkipLen,
                firstChunkReadLen));
    }

    uint32_t remainLen = readLen - firstChunkReadLen;
    uint32_t needChunkNum = DIV_ROUND_UP(remainLen, chunkSize);

    for (uint32_t i = 0; i < needChunkNum; i++)
    {
        uint32_t idx = firstReadIdx + 1 + i;
        if (idx >= writeChunkids.size())
        {
            break;
        }
        uint32_t chunkid = writeChunkids[idx];
        uint32_t chunkWriteLen = chunkSize;

        if (idx + 1 == writeChunkids.size())
        {
            // 最后一个chunk写入的长度
            MetaInfo* pMtInfo = calcMetaInfoPtr(chunkid);
            chunkWriteLen = chunkSize - pMtInfo->m_idleLen;
        }
        
        uint32_t chunkReadLen = std::min(chunkSize, remainLen);
        readInfo.push_back(std::pair<uint64_t, uint32_t>(calcOffset(chunkid), chunkReadLen));
        remainLen -= chunkReadLen;
    }
}

uint32_t EdgeFS::generateTaskid()
{
    return m_nextTaskid++;
}

void EdgeFS::printAllMetaInfo()
{
    uint32_t cnt = 0;
    linfo("taskid %u start", m_currTaskid);
    for (uint32_t chunkid = 0; chunkid < m_pFSHead->m_chunkNum; chunkid++)
    {
        MetaInfo* pMtInfo = calcMetaInfoPtr(chunkid);
        if (!pMtInfo->m_isUsed)
        {
            continue;
        }
        cnt += 1;
        linfo("taskid %u chunkid %u writeLen %u idleLen %u nextChunkId %d", m_currTaskid, chunkid,
            m_pFSHead->m_chunkSize - pMtInfo->m_idleLen, pMtInfo->m_idleLen,
            pMtInfo->m_nextChunkid);
    }
    linfo("taskid %u end, cnt %u", m_currTaskid, cnt);
}
