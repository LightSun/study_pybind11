#ifndef C_COMMON_H
#define C_COMMON_H

#include <inttypes.h>

#ifndef sint8
typedef signed char sint8;
#endif
#ifndef uint8
typedef unsigned char uint8;
#endif

#ifndef sint16
typedef signed short sint16;
#endif
#ifndef uint16
typedef unsigned short uint16;
#endif

#ifndef sint32
typedef signed int sint32;
#endif
#ifndef uint32
typedef unsigned int uint32;
#endif

#ifndef sint64
typedef signed long long sint64;
#endif

#ifndef uint64
#ifdef uint64_t
typedef uint64_t uint64;
#else
//typedef unsigned long long uint64;
typedef uint64_t uint64;
#endif
#endif

#ifdef __cplusplus
#define CPP_START extern "C" {
#define CPP_END }
#else
#define CPP_START
#define CPP_END
#endif

#ifdef WIN32
#define NEW_LINE "\r\n"
#else
#define NEW_LINE "\n"
#endif

#ifdef _WIN32
#define DIR_SEP       '\\'
#define DIR_SEP_STR   "\\"
#else
#define DIR_SEP       '/'
#define DIR_SEP_STR   "/"
#endif

#define H7_EXPORT  __attribute__((visibility("default")))

/*
#ifdef __ANDROID_NDK__
    #include <android/log.h>
    #ifndef LOG_TAG
    #define LOG_TAG "h7"
    #endif
    #define LOGD(fmt, ...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, fmt, ##__VA_ARGS__)
    #define LOGV(fmt, ...) __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, fmt, ##__VA_ARGS__)
    #define LOGW(fmt, ...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, fmt, ##__VA_ARGS__)
#else
    #define LOGD(fmt, ...) printf(fmt, ##__VA_ARGS__)
    #define LOGV(fmt, ...) printf(fmt, ##__VA_ARGS__)
    #define LOGW(fmt, ...) printf(fmt, ##__VA_ARGS__)
#endif
*/

#endif // C_COMMON_H
