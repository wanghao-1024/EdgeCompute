#include "DataMgr.h"
#include "EdgeFSConst.h"
#include "./common/macro.h"

DataMgr::DataMgr()
{
    m_pFileOper = new FileOper();
}

DataMgr::~DataMgr()
{
    SAFE_DELETE(m_pFileOper);
}

void DataMgr::initDataMgr(const std::string& rootDir)
{
    std::string filePath = rootDir + "/" + kDataFileName;
    
    m_pFileOper->setPath(filePath);
    m_pFileOper->open();
}


bool DataMgr::write(const char* buff, uint32_t len, uint64_t offset)
{
    // TODO 可以做按照4K的倍数写入的优化
    // todo 这里需要继续修改FileOper::write的返回值
    return m_pFileOper->write(buff, len, offset);
}

bool DataMgr::read(char* buff, uint32_t len, uint64_t offset)
{
    return m_pFileOper->read(buff, len, offset);
}
