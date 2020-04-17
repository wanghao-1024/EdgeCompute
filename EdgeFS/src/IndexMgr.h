#pragma once

#include "./common/FileOper.h"
#include <string>

class IndexMgr : public FileOper
{
public:
    IndexMgr();
    ~IndexMgr();

public:
    bool initIndexMgr(const std::string& rootDir, uint64_t fileSize, bool& isExistIdxFile);
};