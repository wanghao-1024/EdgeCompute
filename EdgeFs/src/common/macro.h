#pragma once

#ifdef  _WIN64
#define struct_offsetof(s,m)   (size_t)( (ptrdiff_t)&reinterpret_cast<const volatile char&>((((s *)0)->m)) )
#else
#define struct_offsetof(s,m)   (size_t)&reinterpret_cast<const volatile char&>((((s *)0)->m))
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(array) (sizeof(array)/sizeof(array[0]))
#endif

#ifndef DIV_ROUND_UP
#define DIV_ROUND_UP(n,d) (((n) + (d) - 1) / (d))
#endif

#ifndef DIV_ROUND_DOWN
#define DIV_ROUND_DOWN(n,d) ((n) / (d))
#endif

#ifndef ROUND_DOWN
#define ROUND_DOWN(n,d) ((n) / (d) * (d))
#endif

// d必须是2的倍数
#ifndef ALIGN_DOWN
#define ALIGN_DOWN(n, d) (n & (~(d-1)))
#endif

#ifndef ALIGN_UP
#define alignment_up(n, d) ((n+d-1) & (~(d-1))) 
#endif

#define SAFE_DELETE(ptr) \
    do \
    { \
        if (NULL != ptr) \
        { \
            delete ptr; \
            ptr = NULL; \
        } \
    } while(0)

#define SAFE_COLSE(fd) \
    do \
    { \
        if (fd >= 0) \
        { \
            ::close(fd); \
            fd = -1; \
        } \
    } while(0) \

#define RETURN_IF_TRUE(val) \
    do \
    { \
        if (val) \
        { \
            return ; \
        } \
    } while (0)
    
#define ENUM_CASE_TO_STRING(x) case x : return (#x)
