#pragma once

#include <string>
#include <vector>
#include <iostream>

namespace h7 {
using String=std::string;
using CString=const std::string&;
}

#ifndef MED_ASSERT
#define MED_ASSERT(condition)                                                   \
    do                                                                      \
    {                                                                       \
        if (!(condition))                                                   \
        {                                                                   \
            std::cout << "Assertion failure: " << __FUNCTION__  << " >> " << #condition << std::endl;  \
            abort();                                                        \
        }                                                                   \
    } while (0)
#endif

#ifndef MED_ASSERT_IF
#define MED_ASSERT_IF(pre, condition)                                                   \
    do                                                                      \
    {                                                                       \
        if (pre && !(condition))                                                   \
        {                                                                   \
            std::cout << "Assertion failure: " << __FUNCTION__  << " >> " << #condition << std::endl;  \
            abort();                                                        \
        }                                                                   \
    } while (0)
#endif

#ifndef MED_ASSERT_X
#define MED_ASSERT_X(condition, msg)                                                   \
    do                                                                      \
    {                                                                       \
        if (!(condition))                                                   \
        {                                                                   \
            std::cout << "Assertion failure: " << __FUNCTION__  << " >> "<< #condition << ", " << msg << std::endl;  \
            abort();                                                        \
        }                                                                   \
    } while (0)
#endif

#define FDASSERT(condition, format, ...)                                       \
  if (!(condition)) {                                                          \
    int n = std::snprintf(nullptr, 0, format, ##__VA_ARGS__);                  \
    std::vector<char> buffer(n + 1);                                           \
    std::snprintf(buffer.data(), n + 1, format, ##__VA_ARGS__);                \
    std::cout << buffer.data() << std::endl;                                     \
    std::abort();                                                              \
  }

#ifndef HMAX
#define HMAX(a, b) (a > b ? a : b)
#define HMIN(a, b) (a < b ? a : b)
#endif

#ifdef WIN32
#define NEW_LINE "\r\n"
const char SEP = '\\';
#else
const char SEP = '/';
#define NEW_LINE "\n"
#endif

template <typename ...Args>
static inline void println(const std::string& fmt, Args&&... args){
    char buf[256];
    snprintf(buf, 256, fmt.c_str(), std::forward<Args>(args)...);
    std::cout << buf << std::endl;
}

