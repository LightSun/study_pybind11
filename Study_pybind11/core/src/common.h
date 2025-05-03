#pragma once

#ifdef __cplusplus
extern "C" {
#endif
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
}
#endif

#include <algorithm>
#include <functional>
#include <iostream>
#include <vector>
#include <future>
#include <string>

namespace h7 {
using CString = const std::string&;
using String = std::string;
}
#ifndef HMIN
#define HMIN(a, b) (a < b ? a : b)
#endif

#ifndef HMAX
#define HMAX(a, b) (a > b ? a : b)
#endif

#if defined(_WIN32) || defined(WIN32)
#define CMD_LINE "\r\n"
#else
#define CMD_LINE "\n"
#endif

#if defined(_WIN32)|| defined(WIN32)
    #include <windows.h>
#define SLEEP(x) Sleep(x) //msec
#else
    #include <unistd.h>
#define SLEEP(x) usleep(x*1000)
#endif

#define DEFAULT_SEQ '\t'

#define DELETE_P(p) if(p){delete p; p = NULL;}
#define COPY_MEMBER(src, dst, name) dst.name = src.name

#define FUNC_PLACEHOLDERS_1 std::placeholders::_1
#define FUNC_PLACEHOLDERS_2 FUNC_PLACEHOLDERS_1,std::placeholders::_2
#define FUNC_PLACEHOLDERS_3 FUNC_PLACEHOLDERS_2,std::placeholders::_3
#define FUNC_PLACEHOLDERS_4 FUNC_PLACEHOLDERS_3,std::placeholders::_4
#define FUNC_PLACEHOLDERS_5 FUNC_PLACEHOLDERS_4,std::placeholders::_5
#define FUNC_PLACEHOLDERS_6 FUNC_PLACEHOLDERS_5,std::placeholders::_6
#define FUNC_PLACEHOLDERS_7 FUNC_PLACEHOLDERS_6,std::placeholders::_7
#define FUNC_PLACEHOLDERS_8 FUNC_PLACEHOLDERS_7,std::placeholders::_8
#define FUNC_PLACEHOLDERS_9 FUNC_PLACEHOLDERS_8,std::placeholders::_9

#define FUNC_SHARED_PTR(func_expre) std::shared_ptr<std::packaged_task<func_expre>>
#define FUNC_MAKE_SHARED_PTR(func, func_expre, place_holders) \
std::make_shared<std::packaged_task<func_expre>>(std::bind(func, place_holders))

#define FUNC_MAKE_SHARED_PTR_0(func_expre, func) std::make_shared<std::packaged_task<func_expre>>(std::bind(func))
#define FUNC_MAKE_SHARED_PTR_1(func_expre, func) FUNC_MAKE_SHARED_PTR(func, func_expre, FUNC_PLACEHOLDERS_1)
#define FUNC_MAKE_SHARED_PTR_2(func_expre, func) FUNC_MAKE_SHARED_PTR(func, func_expre, FUNC_PLACEHOLDERS_2)
#define FUNC_MAKE_SHARED_PTR_3(func_expre, func) FUNC_MAKE_SHARED_PTR(func, func_expre, FUNC_PLACEHOLDERS_3)
#define FUNC_MAKE_SHARED_PTR_4(func_expre, func) FUNC_MAKE_SHARED_PTR(func, func_expre, FUNC_PLACEHOLDERS_4)
#define FUNC_MAKE_SHARED_PTR_5(func_expre, func) FUNC_MAKE_SHARED_PTR(func, func_expre, FUNC_PLACEHOLDERS_5)
#define FUNC_MAKE_SHARED_PTR_6(func_expre, func) FUNC_MAKE_SHARED_PTR(func, func_expre, FUNC_PLACEHOLDERS_6)
#define FUNC_MAKE_SHARED_PTR_7(func_expre, func) FUNC_MAKE_SHARED_PTR(func, func_expre, FUNC_PLACEHOLDERS_7)
#define FUNC_MAKE_SHARED_PTR_8(func_expre, func) FUNC_MAKE_SHARED_PTR(func, func_expre, FUNC_PLACEHOLDERS_8)
#define FUNC_MAKE_SHARED_PTR_9(func_expre, func) FUNC_MAKE_SHARED_PTR(func, func_expre, FUNC_PLACEHOLDERS_9)

#define HFMT_BUF_32(code, fmt,...)\
do{\
    char buf[32];\
    snprintf(buf, 32, fmt, ##__VA_ARGS__);\
    code;\
}while(0);

#define HFMT_BUF_64(code, fmt, ...)\
do{\
    char buf[64];\
    snprintf(buf, 64, fmt, ##__VA_ARGS__);\
    code;\
}while(0);

#define HFMT_BUF_128(code, fmt, ...)\
do{\
    char buf[128];\
    snprintf(buf, 128, fmt, ##__VA_ARGS__);\
    code;\
}while(0);

#define HFMT_BUF_256(code, fmt ,...)\
do{\
    char buf[256];\
    snprintf(buf, 256, fmt, ##__VA_ARGS__);\
    code;\
}while(0);

#ifndef MED_ASSERT
#define MED_ASSERT(condition)                                                   \
    do                                                                      \
    {                                                                       \
        if (!(condition))                                                   \
        {                                                                   \
            std::cout << "Assertion failure: " << __FILE__ << "::" << __FUNCTION__  \
                                     << __LINE__ \
                                     << " >> " << #condition << std::endl;  \
            abort();                                                        \
        }                                                                   \
    } while (0)
#endif

#ifndef MED_ASSERT_IF
#define MED_ASSERT_IF(pre, condition)                                       \
    do                                                                      \
    {                                                                       \
        if (pre && !(condition))                                            \
        {                                                                   \
            std::cout << "Assertion failure: " << __FILE__ << "::" << __FUNCTION__  \
                                     << __LINE__ \
                                     << " >> " << #condition << std::endl;  \
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
            std::cout << "Assertion failure: " << __FILE__ << "::" << __FUNCTION__  \
                                     << __LINE__ \
                                     << "\n" << msg << " >> " << #condition << std::endl;  \
            abort();                                                        \
        }                                                                   \
    } while (0)
#endif

#define PRINTLN(fmt, ...) printf(fmt, ##__VA_ARGS__)
#define PRINTERR(fmt, ...) fprintf(stderr,fmt, ##__VA_ARGS__)

#define LOGE(fmt, ...) fprintf(stderr,fmt, ##__VA_ARGS__)
#define LOGI(fmt, ...) printf(fmt, ##__VA_ARGS__)
#define LOGW(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__)

#define CMP_AESC(x, y) (x > y ? 1: (x != y ? -1 : 0))
#define CMP_DESC(x, y) (x > y ? -1: (x != y ? 1 : 0))

#define H7_DISALLOW_COPY_AND_MOVE(TypeName)                                                                           \
  TypeName(const TypeName &) = delete;                                                                                 \
  TypeName & operator=(const TypeName &) = delete;                                                                     \
  TypeName(TypeName &&) = delete;                                                                                      \
  TypeName & operator=(TypeName &&) = delete

namespace h7 {

//class INoCopy
//{
//protected:
//    INoCopy() = default;
//    virtual ~INoCopy() = default;
//    INoCopy(INoCopy const& other) = delete;
//    INoCopy& operator=(INoCopy const& other) = delete;
//    INoCopy(INoCopy&& other) = delete;
//    INoCopy& operator=(INoCopy&& other) = delete;
//};

template<bool B, class T = void>
    struct user_enable_if {};

template<class T>
struct user_enable_if<true, T> { typedef T type; };


template<typename T>
struct Traits{
    static const bool is_basic = true;
};

//struct A{};
//template<>
//struct Traits<A>{
//    static const bool is_basic = false;
//};

}
//template<typename T>
//typename user_enable_if<Traits<T>::is_basic, T>::type f(T a){
//    cout<<"a basic type"<<endl;
//    return a;
//}
//template<typename T>
//typename user_enable_if<!Traits<T>::is_basic, T>::type f(T a){
//    cout<<"a class type"<<endl;
//    return a;
//}
