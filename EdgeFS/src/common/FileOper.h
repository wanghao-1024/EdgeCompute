#pragma once
#include "common.h"

class FileOper
{
public:
    FileOper();
    FileOper(const std::string &path);
    ~FileOper();

public:
    void setPath(const std::string &path);

    bool write(const char* buff, uint32_t len, uint64_t offset);
    bool write(const char* buff, uint32_t len);

    bool read(char* buff, uint32_t len, uint64_t offset);
    bool read(char* buff, uint32_t len);

    void close();
    
    bool open(int oflag = O_RDWR | O_CREAT);

public:
    const std::string& getPath()    { return m_path; }

    int getfd();

private:
    int             m_fd;
    std::string     m_path;
};