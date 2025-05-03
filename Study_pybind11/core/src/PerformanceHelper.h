#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <chrono>
#include <sstream>

namespace h7 {

class PerformanceHelper
{
public:
    typedef std::chrono::time_point<std::chrono::steady_clock> ClockTimeUnit;
    typedef std::chrono::steady_clock DelayClock;

    //time in ms
    static std::string formatTime(float time){
        char buf[32];
        float val = time;
        if (val < 1000) {
            snprintf(buf, 32, "%.2f msec", val);
            return std::string(buf);
        }
        val = val / 1000;
        if (val < 60) {
            snprintf(buf, 32, "%.2f sec", val);
            return std::string(buf);
        }
        float format = 60;
        val = val / format;
        if (val < format) {
            snprintf(buf, 32, "%.2f min", val);
            return std::string(buf);
        }
        val = val / format;
        if (val < format) {
            snprintf(buf, 32, "%.2f hour", val);
            return std::string(buf);
        }
        format = 24;
        val = val / format;
        snprintf(buf, 32, "%.2f day", val);
        return std::string(buf);
    }

    inline void begin(){
        m_start = DelayClock::now();
    }
    //return cost in ms
    inline float end(){
        return std::chrono::duration<float, std::milli>(
                     DelayClock::now() - m_start).count();
    }
    inline void print(const std::string& tag){
        float time = end();
        std::string str = formatTime(time);
        printf("%s >> cost %s.\n", tag.c_str(), str.c_str());
        m_start = DelayClock::now();
    }
    inline void printTo(const std::string& tag, std::stringstream& out){
        float time = end();
        std::string str = formatTime(time);
        char buf[128];
        snprintf(buf, 128, "%s >> cost %s.\n", tag.c_str(), str.c_str());
        out << buf;
    }
    inline void printTo(const std::string& tag, std::ostringstream& out){
        float time = end();
        std::string str = formatTime(time);
        char buf[128];
        snprintf(buf, 128, "%s >> cost %s.\n", tag.c_str(), str.c_str());
        out << buf;
    }
    inline void printTo(const std::string& tag, std::string& out){
        float time = end();
        std::string str = formatTime(time);
        char buf[128];
        snprintf(buf, 128, "%s >> cost %s.\n", tag.c_str(), str.c_str());
        out += buf;
    }
    //istringstream can convert string to int/float or etc. like '52.1'

private:
    ClockTimeUnit m_start;
};

typedef PerformanceHelper PerfHelper;
}
