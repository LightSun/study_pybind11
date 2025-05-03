
#include <string>
#include <vector>
#include <iostream>
#include <fstream>

namespace EncDecPy {

using CString = const std::string&;
using String = std::string;

using sint64 = long long;
using uint64 = unsigned long long;

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
        if(len > 0 && m_file != nullptr){
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

static inline std::string getFileContent0(CString file){
    EncDecPy::FileInput fin(file);
    if(!fin.is_open()){
        return "";
    }
    String buf;
    buf.resize(fin.getLength());
    fin.reset();
    fin.read((void*)buf.data(), buf.length());
    fin.close();
    return buf;
}
}
