#include "DataMgr.h"
#include "EdgeFSConst.h"
#include "./common/macro.h"

DataMgr::DataMgr()
{
}

DataMgr::~DataMgr()
{
}

bool DataMgr::initDataMgr(const std::string& rootDir, uint64_t fileSize)
{
    std::string filePath = rootDir + "/" + kDataFileName;
    
    FileOper::setPath(filePath);

    if (!FileOper::open())
    {
        lerror("initDataMgr failed, open data file failed, filepath %s", filePath.c_str());
        return false;
    }

    int dataFd = FileOper::getfd();
    if (ftruncate(dataFd, fileSize))
    {
        lfatal("initDataMgr failed, ftruncate failed, diskSize %" PRIu64 " err %s", fileSize, strerror(errno));
        return false;
    }
    linfo("initDataMgr ftruncate data file, dataFd %d diskSize %" PRIu64, dataFd, fileSize);
    return true;
}

bool DataMgr::write(const char* buff, uint32_t len, uint64_t offset)
{
    // TODO 可以做按照4K的倍数写入的优化
    return FileOper::write(buff, len, offset);
}
