#include "../src/IEdgeFS.h"
#include "../src/common/common.h"
#include <stdio.h>
#include <unistd.h>
#include <inttypes.h>
#include <string.h>

int main(int argc, char* argv[])
{
    if (argc < 3)
    {
        printf("./edge_write_main filename filesize\n");
        return -1;
    }

    IEdgeFS* pEfsHandler = CreateEdgeFS();

    SystemInfo sinfo;
    sinfo.m_diskCapacity = 1024ull * 1024 * 1024 * 100; // 100GB
    sinfo.m_diskRootDir = "../data/";
    sinfo.m_edgeFSUsableMemory = 1024 * 1024 * 100;    // 100MB

    std::string filename = std::string(argv[1]);
    std::string writeFilename = "read_" + filename;
    uint64_t fileSize = strtoll(argv[2], NULL, 10);

    printf("filename %s fileSize %" PRIu64 "\n", filename.c_str(), fileSize);

    unlink(writeFilename.c_str());

    uint64_t writeLen = 0;
    char *pBuff = new char[1024*1024*10];  // 10M
    FileOper* pFileOper = new FileOper(writeFilename);
    if (!pFileOper->open())
    {
        printf("open failed\n");
        goto ERR;
    }

    if (!pEfsHandler->initFS(sinfo))
    {
        printf("init fs failed\n");
        goto ERR;
    }

    static std::random_device r;
    static std::default_random_engine e(r());
    static std::uniform_int_distribution<uint32_t> dist(0, 1024*1024*5+100);
    
    while (writeLen < fileSize)
    {
        uint32_t readLen = dist(e);
        //readLen = 1024 * 1024 * 1;
        readLen = std::min((uint64_t)readLen, fileSize - writeLen);

        printf("EdgeFS::read offset %" PRIu64 " len %u fileSize %" PRIu64 "\n", writeLen, readLen, fileSize);
        int64_t ret = pEfsHandler->read(filename, pBuff, readLen, writeLen);
        if (ret != readLen)
        {
            printf("EdgeFS::read failed, readLen %u ret %" PRId64 " offset %" PRIu64 "\n", readLen, ret, writeLen);
            goto ERR;
        }
        if (!pFileOper->write(pBuff, readLen))
        {
            printf("read failed, readLen %u, fileSize %" PRIu64 " writeLen %" PRIu64 "\n", readLen, fileSize, writeLen);
            goto ERR;
        }
        writeLen += ret;
    }
    printf("sleep\n");
ERR:
    sleep(3);
    if (NULL != pFileOper)
    {
        delete pFileOper;
    }
    if (NULL != pBuff)
    {
        delete[] pBuff;
    }
    DestroyPcdnSdk(pEfsHandler);
    return 0;
}