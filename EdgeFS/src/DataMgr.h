#pragma once

#include "./common/FileOper.h"
#include <string>

class DataMgr : public FileOper
{
public:
    DataMgr();
    ~DataMgr();

public:
    bool initDataMgr(const std::string& rootDir, uint64_t fileSize);

    virtual bool write(const char* buff, uint32_t len, uint64_t offset);
};