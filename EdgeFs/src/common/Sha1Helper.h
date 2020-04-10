#pragma once

#include "sha1.h"
#include <stdio.h>
#include <stdint.h>
#include <string>

#define SHA_DIGEST_LENGTH 20

class ShaHelper
{
public:
    static void calcShaToHex(const std::string& data, char* digest)
    {
        calcShaToHex(data.c_str(), data.size(), digest);
    }

    static void calcShaToHex(const char* data, uint64_t dataSize, char* digest)
    {
        SHA_INFO ctx;
        sha_init(&ctx);
        sha_update(&ctx, (SHA_BYTE*)data, (int)dataSize);
        sha_final((unsigned char*)digest, &ctx);
    }

    static std::string calShaToHex(const std::string& data)
    {
        return calShaToHex(data.c_str(), data.size());
    }

    static std::string calShaToHex(const char* data, uint64_t dataSize)
    {
        std::string shaResult;
        unsigned char digest[SHA_DIGEST_LENGTH] = { '\0' };
        SHA_INFO ctx;

        sha_init(&ctx);
        sha_update(&ctx, (SHA_BYTE*)data, (int)dataSize);
        sha_final(digest, &ctx);

        shaResult.assign((char*)digest, SHA_DIGEST_LENGTH);
        return shaStrToHex(shaResult);
    }

    static std::string calFileShaToHex(const char* fileName, int64_t offset = 0)
    {
        std::string shaResult;
        unsigned char digest[SHA_DIGEST_LENGTH] = { '\0' };
        const static size_t buffLen = 4096;
        size_t len = 0;
        char buff[buffLen] = { '\0'};
        SHA_INFO ctx;
        FILE* pHandler = NULL;

        pHandler = fopen(fileName, "rb");
        if (NULL == pHandler)
        {
            return shaResult;
        }
        if (offset != 0 &&
                (fseek(pHandler, offset, SEEK_SET) != 0))
        {
            return shaResult;
        }

        sha_init(&ctx);
        while ( (len = fread(buff, sizeof(char), buffLen, pHandler) ) > 0)
        {
            sha_update(&ctx, (SHA_BYTE*)buff, (int)len);
        }
        fclose(pHandler);
        sha_final(digest, &ctx);

        shaResult.assign((char*)digest, SHA_DIGEST_LENGTH);
        return shaStrToHex(shaResult);
    }

public:
    static std::string shaStrToHex(const std::string& in)
    {
        std::string out;
        if (0 != shaStrToHex(in, out))
        {
            out.clear();
        }
        return out;
    }

    static std::string shaHexToStr(const std::string& in)
    {
        std::string out;
        if (0 != shaHexToStr(in, out))
        {
            out.clear();
        }
        return out;
    }

public:

    static int32_t shaStrToHex(const std::string& in, std::string& out)
    {
        out.clear();
        std::string::const_iterator it;
        char tmp[4] = {'\0'};
        for ( it = in.begin(); it != in.end(); it++ )
        {
            sprintf(tmp, "%02x", (unsigned char)(*it));
            out += tmp;
        }

        return 0;
    }

    static std::string shaStrToHex(const unsigned char* src, int32_t len)
    {
        std::string out;
        char tmp[4] = {'\0'};
        for (int32_t i = 0; i < len; i++)
        {
            sprintf(tmp, "%02x", src[i]);
            out += tmp;
        }
        return out;
    }

    static int32_t shaHexToStr(const std::string& in, std::string& out)
    {
        if (0 != in.size() % 2)
        {
            return -1;
        }

        out.clear();
        uint8_t high, low;
        unsigned char tmp;
        std::string::const_iterator it;
        for ( it = in.begin(); it != in.end(); it += 2 )
        {
            high = *it;
            if (high >= '0' && high <= '9')
            {
                high = high - '0';
            }
            else if (high>='A' && high<='F')
            {
                high = high - 'A' + 10;
            }
            else if (high>='a' && high<='f')
            {
                high = high - 'a' + 10;
            }
            else
            {
                out.clear();
                return -1;
            }

            low  = *(it+1);
            if (low >= '0' && low <= '9')
            {
                low = low - '0';
            }
            else if (low>='A' && low<='F')
            {
                low = low - 'A' + 10;
            }
            else if (low>='a' && low<='f')
            {
                low = low - 'a' + 10;
            }
            else
            {
                out.clear();
                return -1;
            }

            tmp = high << 4 | low;

            out.push_back((char)tmp);
        }
        return 0;
    }
};
