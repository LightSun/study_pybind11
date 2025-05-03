#pragma once

#if __cplusplus >= 201703L
    #include <string_view>
#endif
#include "core/src/common.h"

#define __DEF_STRING_BUFFER_READ(type, Type)\
    bool read##Type(type* out){\
        if(getLeftLength() >= sizeof (type)){\
            *out = *(type*)getDataPtr();\
            return true;\
        }\
        return false;\
    }

namespace h7 {

class StringBuffer{
public:

    using SB_String = std::string;

    StringBuffer(CString buf):m_data(buf){}

    bool readLine(String& str){
        str.clear();
        char ch;
        while (m_pos < m_data.length()) {
            ch = m_data.data()[m_pos++];
            if (ch == '\n') {
                // unix: LF
                return true;
            }
            if (ch == '\r') {
                // dos: CRLF
                // read LF
                ch = m_data.data()[m_pos++];
                if(ch != '\n'){
                    m_pos--;
                }
                return true;
            }
            str += ch;
        }
        return str.length() != 0;
    }

    SB_String readLine(){
        if(m_pos >= m_data.length()){
            return "";
        }
        const size_t start_pos = m_pos;
        char ch;
        while (m_pos < m_data.length()) {
            ch = m_data.data()[m_pos++];
            if (ch == '\n') {
                // unix: LF
                break;
            }
            if (ch == '\r') {
                // dos: CRLF
                // read LF
                ch = m_data.data()[m_pos++];
                if(ch != '\n'){
                    m_pos--;
                }
                break;
            }
        }
        if(m_pos == start_pos){
            return "";
        }
        return SB_String(m_data.data() + start_pos,
                         m_pos - start_pos);
    }

    void reset(){
        m_pos = 0;
    }
    size_t getLeftLength(){
        return m_data.length() - m_pos;
    }
    char* getDataPtr(){
        return (char*)m_data.data() + m_pos;
    }
    __DEF_STRING_BUFFER_READ(char, Char)
    __DEF_STRING_BUFFER_READ(short, Short)
    __DEF_STRING_BUFFER_READ(int, Int)
    __DEF_STRING_BUFFER_READ(long long, Long)
    __DEF_STRING_BUFFER_READ(unsigned char, UChar)
    __DEF_STRING_BUFFER_READ(unsigned short, UShort)
    __DEF_STRING_BUFFER_READ(unsigned int, UInt)
    __DEF_STRING_BUFFER_READ(unsigned long long, ULong)

    SB_String readString(size_t len){
        if(getLeftLength() >= len){
             SB_String ret(getDataPtr(), len);
             m_pos += len;
             return ret;
        }
        return "";
    }

private:
    String m_data;
    unsigned int m_pos {0};
};
}
