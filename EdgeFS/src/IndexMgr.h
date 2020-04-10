#pragma once

#include "./common/FileOper.h"
#include <string>

class IndexMgr
{
public:
    IndexMgr();
    ~IndexMgr();

public:
    void initIndexMgr(const std::string& rootDir, bool& isExistIdxFile);
    
public:
    int getfd()
    {
        return m_pFileOper->getfd();
    }

private:
    FileOper*       m_pFileOper;
};