#pragma once
#include <stdint.h>

class Utils
{
public:
    template<typename T>
    static void limit(T& val, T min, T max)
    {
        if (val < min)
        {
            val = min;
        }
        if (val > max)
        {
            val = max;
        }
    }
};