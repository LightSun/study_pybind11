#pragma once

#ifdef _WIN32
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#else
#include <unistd.h>
#endif

#ifdef _WIN32
#define GET_PID GetCurrentProcessId
#define GET_TID GetCurrentThreadId

static inline int setenv(const char *name, const char *value, int overwrite)
{
    if(!overwrite) {
        size_t envsize = 0;
        char buffer[1024];
        int code = GetEnvironmentVariable(name, buffer, 1024);
        //all-ready exist.
        if(code != 0){
            return 0;
        }
    }
    return _putenv_s(name, value);
}

#else
#define GET_PID getpid
#define GET_TID gettid
#endif

