#pragma once

#include <vector>
#include <string>
#include <memory.h>
#include <unordered_map>

namespace h7 {

    using String = std::string;
    using CString = const String&;
    using sint64 = long long;
    using uint64  = size_t;
    using uint16 = unsigned short;
    using uint32 = unsigned int;

#define __k8N(x) ((x + 8 - 1) & (~(8 - 1)))

#define __BBO_PUT0(name, type)\
        void put##name(type val){\
            prepareBufferIncIfNeed(sizeof (type));\
            memcpy(m_buffer.data() + m_pos, &val, sizeof (type));\
            m_pos += sizeof (type);\
        }

#define __BIO_GET0(name, type)\
    type get##name(){\
        type val;\
        memcpy(&val, m_buf->data() + m_pos, sizeof (type));\
        m_pos += sizeof (type);\
        return val;\
    }

    class ByteBufferOut{
    public:
        ByteBufferOut(uint64 size){
            prepareBuffer(size);
        }
        ByteBufferOut(){}
        __BBO_PUT0(Byte, char);
        __BBO_PUT0(Int, int);
        __BBO_PUT0(Short, short);
        __BBO_PUT0(Long, sint64);
        __BBO_PUT0(Bool, bool);
        __BBO_PUT0(Float, float);
        __BBO_PUT0(Double, double);

        __BBO_PUT0(UByte, unsigned char);
        __BBO_PUT0(UShort, unsigned short);
        __BBO_PUT0(UInt, unsigned int);
        __BBO_PUT0(ULong, uint64);

        void putData(const void* data, uint64 len){
            prepareBufferIncIfNeed(len);
            memcpy(m_buffer.data() + m_pos, data, len);
            m_pos += len;
        }
        void putRawString(CString str){
            putData(str.data(), str.length());
        }
        void putString16(CString str){
            prepareBufferIncIfNeed(sizeof (uint16) + str.length());
            putUShort(str.length());
            if(str.length() > 0){
                putData(str.data(), str.length());
            }
        }

        void putString(CString str){
            prepareBufferIncIfNeed(sizeof (uint32) + str.length());
            putUInt(str.length());
            if(str.length() > 0){
                putData(str.data(), str.length());
            }
        }
        void putString64(CString str){
            prepareBufferIncIfNeed(sizeof (uint64) + str.length());
            putULong(str.length());
            if(str.length() > 0){
                putData(str.data(), str.length());
            }
        }
        void putListInt(const std::vector<int>& vec){
            prepareBufferIncIfNeed(vec.size() * sizeof (int));
            uint64 data_size = vec.size() * sizeof (int);
            putUInt(vec.size());
            putData(vec.data(), data_size);
        }
        void putMap(const std::unordered_map<char, int>& out){
            prepareBufferIncIfNeed(64);
            putInt(out.size());
            //
            auto it = out.begin();
            for(; it != out.end(); ++it){
                putByte(it->first);
                putInt(it->second);
            }
        }
        //------------------------
        uint64 getLength(){
            return m_pos;
        }
        void bufferToString(String& out){
            out.append(m_buffer.data(), m_pos);
        }
        String bufferToString(){
            String str(m_buffer.data(), m_pos);
            return str;
        }
        void prepareBuffer(uint64 len){
            m_buffer.resize(__k8N(len));
        }
        void prepareBufferInc(uint64 len){
            prepareBuffer(m_buffer.size() + len);
        }
        void prepareBufferIncIfNeed(uint64 len){
            auto total = m_buffer.size();
            if(m_pos + len > total){
                if(m_autoInc){
                    prepareBufferInc(m_pos + len - total);
                }else{
                    fprintf(stderr, "prepareBufferIncIfNeed failed for autoInc = false.\n");
                    abort();
                    //ASSERT(false, "prepareBufferIncIfNeed failed for autoInc = false.");
                }
            }
        }
        void setBufferAutoInc(bool _auto){
            m_autoInc = _auto;
        }
        char* data(){
            return m_buffer.data();
        }
    private:
        std::vector<char> m_buffer;
        uint64 m_pos{0};
        bool m_autoInc {true};
    };

    class ByteBufferIO{
    public:
        ByteBufferIO(String* str): m_buf(str), m_needFree(false){
        }
        ByteBufferIO(CString str): m_needFree(true){
            m_buf = new String();
            m_buf->resize(str.length());
            *m_buf = str;
        }
        ~ByteBufferIO(){
            if(m_needFree && m_buf){
                delete m_buf;
                m_buf = nullptr;
            }
        }
        void append(CString str){
            m_buf->append(str);
        }
        void append(const void* data, size_t len){
            m_buf->append((char*)data, len);
        }
        void trim(){
            if(m_pos > 0){
                uint64 len_left = m_buf->length() - m_pos;
                char* data = (char*)m_buf->data();
                memmove(data, data + m_pos, len_left);
                m_pos = 0;
            }
        }
        void skip(size_t len){
            m_pos += len;
        }
        __BIO_GET0(Byte, char);
        __BIO_GET0(Int, int);
        __BIO_GET0(Short, short);
        __BIO_GET0(Long, sint64);
        __BIO_GET0(Bool, bool);
        __BIO_GET0(Float, float);
        __BIO_GET0(Double, double);

        __BIO_GET0(UByte, unsigned char);
        __BIO_GET0(UShort, unsigned short);
        __BIO_GET0(UInt, unsigned int);
        __BIO_GET0(ULong, uint64);

        void getData(uint64 len, const void* out){
            memcpy((void*)out, getCurDataPtr(), len);
            m_pos += len;
        }
        String getRawString(uint64 len){
            String str;
            str.resize(len);
            getData(len, str.data());
            return str;
        }
        String getString16(){
            auto len = getUShort();
            String str;
            str.resize(len);
            getData(len, str.data());
            return str;
        }
        String getString(){
            auto len = getUInt();
            String str;
            str.resize(len);
            getData(len, str.data());
            return str;
        }
        String getString64(){
            auto len = getULong();
            String str;
            str.resize(len);
            getData(len, str.data());
            return str;
        }
        void getListInt(std::vector<int>& out){
            auto size = getInt();
            for(int i = 0 ; i < size ; ++i){
                out.push_back(getInt());
            }
        }
        void getMap(std::unordered_map<char, int>& out){
            auto size = getInt();
            for(int i = 0 ; i < size ; ++i){
                char key = getByte();
                int val = getInt();
                out[key] = val;
            }
        }

        //--------------------
        uint64 getLeftLength(){
            return m_buf->length() - m_pos;
        }
        uint64 getLength(){
            return m_buf->length();
        }

    private:
        char* getCurDataPtr(){
            return (char*)m_buf->data() + m_pos;
        }
    private:
        String* m_buf;
        uint64 m_pos{0};
        bool m_needFree;
    };
}
