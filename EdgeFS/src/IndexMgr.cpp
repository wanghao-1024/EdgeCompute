#include "./IndexMgr.h"
#include "./EdgeFSConst.h"
#include "./common/macro.h"
#include <string>
#include <unistd.h>

IndexMgr::IndexMgr()
{
    m_pFileOper = new FileOper();
}
IndexMgr::~IndexMgr()
{
    SAFE_DELETE(m_pFileOper);
}

void IndexMgr::initIndexMgr(const std::string& rootDir, bool& isExistIdxFile)
{
    std::string filePath = rootDir + "/" + kIndexFileName;
    m_pFileOper->setPath(filePath);
    isExistIdxFile = 0 == access(m_pFileOper->getPath().c_str(), F_OK) ? true : false;
    m_pFileOper->open();
}