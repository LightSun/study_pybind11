#pragma once

#ifdef _WIN32
#include <time.h>
#include <sstream>
#include <iomanip>
#else
//#include <sys/time.h>
//#include <unistd.h>
#endif
//
#include <iostream>
#include <chrono>
#include <cstdio>
#include <time.h>

namespace h7_handler_os{

using LLong = long long;

//in ms
static inline LLong getCurTime(){
    //#ifdef _WIN32
    //    return clock();
    //#else
    auto now = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               now.time_since_epoch()).count();
    //    struct timeval time;
    //    gettimeofday(&time, NULL);
    //    return time.tv_usec/1000;
    //#endif
}

//
static inline std::string formatTime(long long timeMs){
    //printf("formatTime >> timeMs + %llu\n", timeMs);
    time_t tt = timeMs / 1000;
    //auto localTm = gmtime(&tt);
    auto localTm = localtime(&tt);

    char buff[64];
    int len = strftime(buff, 64, "%Y-%m-%d %H:%M:%S", localTm);
    snprintf(buff + len, 64 - len, ".%03u", (unsigned int)(timeMs % 1000));
    return std::string(buff);
}

static inline std::string getCurTimeStr(){
    return formatTime(getCurTime());
}

#ifdef _WIN32
static inline char* strptime(const char* s,
                             const char* f,
                             struct tm* tm) {
    // Isn't the C++ standard lib nice? std::get_time is defined such that its
    // format parameters are the exact same as strptime. Of course, we have to
    // create a string stream first, and imbue it with the current C locale, and
    // we also have to make sure we return the right things if it fails, or
    // if it succeeds, but this is still far simpler an implementation than any
    // of the versions in any of the C standard libraries.
    std::istringstream input(s);
    input.imbue(std::locale(setlocale(LC_ALL, nullptr)));
    input >> std::get_time(tm, f);
    if (input.fail()) {
        return nullptr;
    }
    return (char*)(s + input.tellg());
}
#endif

static inline long long strToTimeSeconds(const std::string& timeStr){
    struct tm timeinfo;
    strptime(timeStr.data(), "%Y-%m-%d %H:%M:%S",  &timeinfo);
    time_t timeStamp = mktime(&timeinfo);
    //printf("timeStamp=%ld\n",timeStamp);
    return timeStamp;
}

}
