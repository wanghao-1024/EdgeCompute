#pragma once
#include "common.h"

class FileOper
{
public:
    FileOper();
    FileOper(const std::string &path);
    virtual ~FileOper();

public:
    void setPath(const std::string &path);

    virtual bool write(const char* buff, uint32_t len, uint64_t offset);
    virtual bool write(const char* buff, uint32_t len);

    virtual bool read(char* buff, uint32_t len, uint64_t offset);
    virtual bool read(char* buff, uint32_t len);

    virtual void close();
    
    virtual bool open(int oflag = O_RDWR | O_CREAT);

    virtual void unlink();

public:
    const std::string& getPath()    { return m_path; }

    int getfd();

private:
    int             m_fd;
    std::string     m_path;
};