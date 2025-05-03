#pragma once

#include <cstring>
#include "core/src/common.h"
#include "core/src/c_common.h"
#include "core/src/msvc_compat.h"

namespace h7 {

    class FileInput{
    public:
        FileInput(){}
        FileInput(CString path){
            m_file = fopen64(path.data(), "rb");
        }
        ~FileInput(){
            close();
        }
        void open(CString path){
            close();
            m_file = fopen64(path.data(), "rb");
        }
        bool is_open(){
            return m_file != nullptr;
        }
        sint64 getLength(){
            if(m_file == nullptr){
                return 0;
            }
            fseeko64(m_file, 0, SEEK_END);
            return ftello64(m_file);
        }
        void reset(){
            if(m_file != nullptr){
                fseeko64(m_file, 0, SEEK_SET);
            }
        }
        bool seek(uint64 pos, bool end = false){
            if(m_file == nullptr){
                return false;
            }
            if(end){
                fseeko64(m_file, 0, SEEK_END);
            }else{
                fseeko64(m_file, pos, SEEK_SET);
            }
            return true;
        }
        sint64 read(void* data, uint64 len){
            if(m_file == nullptr)
                return -1;
            return fread(data, 1, len, m_file);
        }

        String read(uint64 len){
            MED_ASSERT(len > 0);
            if(m_file != nullptr){
                std::vector<char> vec(len, 0);
                fread(vec.data(), 1, len, m_file);
                return String(vec.data(), len);
            }
            return "";
        }
        bool readline(std::string& str) {
            str.clear();
            char ch;
            while (fread(&ch, 1, 1, m_file)) {
                if (ch == '\n') {
                    // unix: LF
                    return true;
                }
                if (ch == '\r') {
                    // dos: CRLF
                    // read LF
                    if (fread(&ch, 1, 1, m_file) && ch != '\n') {
                        // mac: CR
                        fseek(m_file, -1, SEEK_CUR);
                    }
                    return true;
                }
                str += ch;
            }
            return str.length() != 0;
        }
        std::vector<String> readLines(){
            std::vector<String> ret;
            String line;
            while (readline(line)) {
                ret.push_back(line);
            }
            return ret;
        }
        bool readLastLine(String& str){
           bool ret = readline(str);
           while (readline(str)) {
           }
           return ret;
        }
        bool read2Vec(std::vector<char>& buf){
            if(m_file == nullptr){
                return false;
            }
            buf.resize(getLength());
            reset();
            read(buf.data(), buf.size());
            return true;
        }
        bool read2Str(String& out){
            std::vector<char> buf;
            if(!read2Vec(buf)){
                return false;
            }
            out = String(buf.data(), buf.size());
            return true;
        }
        void close(){
            if(m_file != nullptr){
                fclose(m_file);
                m_file = nullptr;
            }
        }
    private:
        FILE* m_file{nullptr};
    };

    class FileOutput{
    public:
        FileOutput(){}
        FileOutput(CString path){
            m_file = fopen(path.data(), "wb");
        }
        ~FileOutput(){
            close();
        }
        void open(CString path){
            close();
            m_file = fopen(path.data(), "wb");
        }
        bool is_open(){
            return m_file != nullptr;
        }
        void close(){
            if(m_file != nullptr){
                fclose(m_file);
                m_file = nullptr;
            }
        }
        bool writeLine(CString line){
            if(write(line.data(), line.size())){
                newLine();
                return true;
            }
            return false;
        }
        bool write(const void* data, size_t size){
            if(m_file == nullptr){
                return false;
            }
            if(fwrite(data, 1, size, m_file) < size){
                //no left space.
                return false;
            }
            return true;
        }
        void newLine(){
            if(m_file != nullptr){
                String new_line = CMD_LINE;
                fwrite(new_line.data(), 1, new_line.length(), m_file);
            }
        }
        void flush(){
            if(m_file != nullptr){
                fflush(m_file);
            }
        }
    private:
        FILE* m_file{nullptr};
    };

    class BufferFileOutput{
    public:
        BufferFileOutput(uint32 fresh_threshold):m_fresh_thd(fresh_threshold){
        }
        void open(CString path){
            close();
            m_file = fopen64(path.data(), "wb");
        }
        bool is_open(){
            return m_file != nullptr;
        }
        void close(){
            if(m_file != nullptr){
                fclose(m_file);
                m_file = nullptr;
            }
        }
        bool writeLine(CString line){
            if(write(line.data(), line.size())){
                newLine();
                return true;
            }
            return false;
        }
        bool write(CString data){
            return write(data.data(), data.length());
        }
        bool write(const void* data, size_t size){
            if(size == 0){
                return false;
            }
            m_vec.emplace_back(data, size);
            m_cur_size += size;
            if(m_cur_size >= m_fresh_thd){
                return flush();
            }
            return true;
        }
        bool fwriteLine(CString line){
            if(fwrite(line.data(), line.size())){
                fnewLine();
                return true;
            }
            return false;
        }
        bool fwrite(CString data){
            return fwrite(data.data(), data.length());
        }
        bool fwrite(const void* data, size_t size){
            if(m_file == nullptr){
                return false;
            }
            if(std::fwrite(data, 1, size, m_file) < size){
                //no left space.
                return false;
            }
            return true;
        }
        void newLine(){
            String new_line = CMD_LINE;
            write(new_line.data(), new_line.length());
        }
        void fnewLine(){
            if(m_file != nullptr){
                String new_line = CMD_LINE;
                std::fwrite(new_line.data(), 1, new_line.length(), m_file);
            }
        }
        bool flush(){
            for(auto it = m_vec.begin() ; it != m_vec.end() ; ++it){
                if(!fwrite(it->data, it->size)){
                    return false;
                }
            }
            m_vec.clear();
            m_cur_size = 0;
            if(m_file != nullptr){
                fflush(m_file);
            }
            return true;
        }
    private:
        struct Item{
            void* data;
            size_t size;
            Item(const void* _data, size_t _size):size(_size){
                data = malloc(_size);
                memcpy(data, _data, _size);
            }
            ~Item(){
                if(data){
                    free(data);
                    data = nullptr;
                }
            }
        };
        uint32 m_fresh_thd;
        uint32 m_cur_size {0};
        std::vector<Item> m_vec;
        FILE* m_file{nullptr};
    };
}
