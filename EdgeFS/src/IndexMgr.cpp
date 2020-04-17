#include "./IndexMgr.h"
#include "./EdgeFSConst.h"
#include "./common/macro.h"
#include <string>
#include <unistd.h>

IndexMgr::IndexMgr()
{
}
IndexMgr::~IndexMgr()
{
}

bool IndexMgr::initIndexMgr(const std::string& rootDir, uint64_t fileSize, bool& isExistIdxFile)
{
    std::string filePath = rootDir + "/" + kIndexFileName;
    isExistIdxFile = 0 == access(filePath.c_str(), F_OK) ? true : false;

    FileOper::setPath(filePath);
    if (!FileOper::open())
    {
        lerror("initIndexMgr failed, open index file failed, filepath %s isExist %u", filePath.c_str(),
            isExistIdxFile);
        return false;
    }
    if (isExistIdxFile)
    {
        return true;
    }
    
    int fd = FileOper::getfd();
    if (ftruncate(fd, fileSize))
    {
        lfatal("initIndexMgr failed, ftruncate index file failed, fileSize %" PRIu64 " err %s",
            fileSize, strerror(errno));
        return false;
    }
    return true;
}