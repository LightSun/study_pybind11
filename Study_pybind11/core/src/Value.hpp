#pragma once

#include <string>
#include <vector>
#include <stdexcept>
#include <limits>

#define __H7_VALUE_CAST(func, data)\
    try{\
        return std::func(data);\
    }catch(std::invalid_argument const& ex){\
        return def;\
    } catch(std::out_of_range const& ex){\
        return def;\
    }
#define __H7_VALUE_LOW_INT(type)\
    int val = getInt(def);\
    /*if(val > std::numeric_limits<type>::max()\ */\
   /*        || val < std::numeric_limits<type>::min()){\*/\
    /*    return def;\*/\
   /* }\*/\
    return (type)val;

namespace h7 {
class Value{
public:
    using String = std::string;
    using CString = const std::string&;

    Value(CString val):m_val(val){}

    String& get(){
        return m_val;
    }
    unsigned char getUChar(unsigned char def = 0){
        __H7_VALUE_LOW_INT(unsigned char);
    }
    char getChar(char def = -1){
        __H7_VALUE_LOW_INT(char);
    }
    short getShort(short def = -1){
        __H7_VALUE_LOW_INT(short);
    }
    unsigned short getUShort(unsigned short def = 0){
        __H7_VALUE_LOW_INT(unsigned short);
    }
    unsigned int getUInt(unsigned int def = 0){
        __H7_VALUE_CAST(stoul, m_val.data());
    }
    int getInt(int def = 0){
        __H7_VALUE_CAST(stoi, m_val.data());
    }
    long long getLong(long long def = 0){
        __H7_VALUE_CAST(stoll, m_val.data());
    }
    unsigned long long getULong(unsigned long long def = 0){
        __H7_VALUE_CAST(stoull, m_val.data());
    }
    float getFloat(float def = 0){
        __H7_VALUE_CAST(stof, m_val.data());
    }
    double getDouble(double def = 0){
        __H7_VALUE_CAST(stod, m_val.data());
    }
    bool getBool(bool def = false){
        if(m_val.empty()) return def;
        return m_val == "1" || m_val == "true" || m_val == "TRUE";
    }

private:
    String m_val;
};

}
