#include "../src/IEdgeFS.h"
#include <stdio.h>
#include <unistd.h>
#include <inttypes.h>

const std::string testtxt = "0123456789";

int main()
{
    IEdgeFS* efs = CreateEdgeFS();

    SystemInfo sinfo;
    sinfo.m_diskCapacity = 1024ull * 1024 * 1; // 1MB
    sinfo.m_diskRootDir = "../data/";
    sinfo.m_edgeFSUsableMemory = 1024 * 1024 * 1;

    if (!efs->initFS(sinfo))
    {
        printf("init fs failed\n");
        return -1;
    }

    std::string filename1 = "1.txt";
    std::string filename2 = "2.txt";

    for (size_t i = 0; i < 2; i++)
    {
        printf("\nwrite start i %zu\n", i);
        int ret = efs->write(filename1, testtxt.c_str(), testtxt.size());
        printf("write i %zu ret %d\n", i, ret);
    }

    for (size_t i = 0; i < 0; i++)
    {
        printf("\nwrite start i %zu\n", i);
        int ret = efs->write(filename2, testtxt.c_str(), testtxt.size());
        printf("write i %zu ret %d\n", i, ret);
    }

    uint32_t i = 0;
    uint64_t offset = 0;

    while (true)
    {
        printf("\n read i %d filename1 %s\n", i++, filename1.c_str());

        char buff[128+1] = { '\0' };
        int ret = efs->read(filename1, buff, sizeof(buff)-1, offset);
        if (ret < 0)
        {
            printf("err ret %d\n", ret);
            break;
        }
        if (ret == 0)
        {
            break;
        }
        
        buff[128] = '\0';
        printf("read offset %" PRIu64 " readlen %d buff %s\n", offset, ret, buff);
        offset += ret;

        if (ret != 128)
        {
            break;
        }
    }

    return 0;

    i = 0;
    offset = 0;
    while (true)
    {
        printf("\n read i %d filename2 %s\n", i++, filename2.c_str());

        char buff[128+1] = { '\0' };
        int ret = efs->read(filename2, buff, sizeof(buff)-1, offset);
        if (ret < 0)
        {
            printf("err ret %d\n", ret);
            break;
        }
        if (ret == 0)
        {
            break;
        }
        
        buff[128] = '\0';
        printf("read offset %" PRIu64 " readlen %d buff %s\n", offset, ret, buff);
        offset += ret;

        if (ret != 128)
        {
            break;
        }
    }

    DestroyPcdnSdk(efs);
    return 0;
}