#pragma once

#include "./common/FileOper.h"
#include <string>

class DataMgr
{
public:
    DataMgr();
    ~DataMgr();

public:
    void initDataMgr(const std::string& rootDir);

    bool write(const char* buff, uint32_t len, uint64_t offset);

    bool read(char* buff, uint32_t len, uint64_t offset);
    
public:
    int getfd()
    {
        return m_pFileOper->getfd();
    }
    
private:
    FileOper*       m_pFileOper;
};