#include "common.h"

FileOper::FileOper()
: m_fd(-1)
{
}

FileOper::FileOper( const std::string &path )
: m_fd(-1)
, m_path(path)
{    
}

FileOper::~FileOper()
{
    close();
}

bool FileOper::write(const char* buff, uint32_t len)
{
    if (m_fd <= 0)
    {
        return false;
    }

    errno = 0;
    if (len != ::write(m_fd, buff, len))
    {
        lerror("[fileOper] write failed, len %u err %s\n", len, strerror(errno));
        return false;
    }
    return true;
}

bool FileOper::write(const char* buff, uint32_t len, uint64_t offset)
{
    if (m_fd <= 0)
    {
        return false;
    }

    errno = 0;
    if (::lseek(m_fd, offset, SEEK_SET) < 0)
    {
        lerror("[fileOper] lseek failed, offset %" PRIu64 " err %s\n", offset, strerror(errno));
        return false;
    }
    return write(buff, len);
}

bool FileOper::read(char* buff, uint32_t len)
{
    if (m_fd <= 0)
    {
        return false;
    }

    errno = 0;
    int32_t retLen = ::read(m_fd, buff, len);
    if (len != (uint32_t)retLen)
    {
        lerror("[fileOper] read failed, len %u retLen %d fd %d path %s err %s", len, retLen, m_fd, m_path.c_str(),
            strerror(errno));
        return false;
    }

    return true;
}

bool FileOper::read(char* buff, uint32_t len, uint64_t offset)
{
    if (m_fd <= 0)
    {
        return false;
    }

    errno = 0;
    if (::lseek(m_fd, offset, SEEK_SET) < 0)
    {
        lerror("[fileOper] fseek failed, offset %" PRIu64 " err %s\n", offset, strerror(errno));
        return false;
    }
    return read(buff, len);
}

bool FileOper::open(int oflag)
{
    if (m_path.empty())
    {
        return false;
    }

    if (m_fd >= 0)
    {
        return true;
    }

    errno = 0;
    //m_fd = ::open(m_path.c_str(), O_RDWR | O_CREAT | O_LARGEFILE | O_NOATIME, 666);
    m_fd = ::open(m_path.c_str(), oflag, 0666);
    if (m_fd <= 0)
    {
        lerror("open failed, path %s err %s\n", m_path.c_str(), strerror(errno));
        return false;
    }
    return true;
}

void FileOper::close()
{
    if (m_fd >= 0)
    {
        ::close(m_fd);
        m_fd = -1;
    }
}

void FileOper::setPath( const std::string &path )
{
    m_path = path;
}

int FileOper::getfd()
{
    return m_fd;
}

void FileOper::unlink()
{
    ::unlink(m_path.c_str());
}
