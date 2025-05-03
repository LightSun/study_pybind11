#pragma once

#include <vector>
#include <string>
#include <functional>
#include <memory>
#include <mutex>
#include <map>

namespace h7_gz {

using CString = const std::string&;
using String = std::string;

struct IRandomWriter
{
    virtual ~IRandomWriter(){};

    static std::shared_ptr<IRandomWriter> NewFileWriter(CString filePath);

    virtual bool open() = 0;

    virtual void seekTo(size_t /*pos*/) = 0;

    virtual bool write(const String& buf) = 0;

    virtual bool write(const void* data, size_t len) = 0;

    virtual void close() = 0;
};

struct IMemoryWriter : public IRandomWriter{

    virtual String getContent() = 0;
};

struct IDecompressManager{
    virtual ~IDecompressManager(){}
    virtual std::shared_ptr<IRandomWriter> getWriter(CString shortName) = 0;
};

struct FileDecompressManager: public IDecompressManager{

    FileDecompressManager(CString dir):m_outDir(dir){}

    std::shared_ptr<IRandomWriter> getWriter(CString shortName) override{
        String outFile = m_outDir + "/" + shortName;
        return IRandomWriter::NewFileWriter(outFile);
    }
private:
    String m_outDir;
};

struct MemoryDecompressManager: public IDecompressManager{
    using FuncCreateWriter = std::function<std::shared_ptr<IMemoryWriter>()>;

    MemoryDecompressManager(FuncCreateWriter func):m_func(func){}

    std::shared_ptr<IRandomWriter> getWriter(CString shortName) override{
        std::unique_lock<std::mutex> lck(m_mutex);
        auto it = m_maps.find(shortName);
        if(it != m_maps.end()){
            return it->second;
        }
        auto ptr = m_func();
        m_maps[shortName] = ptr;
        return ptr;
    }
    std::map<String,String> getItemMap(){
        std::map<String,String> ret;
        std::unique_lock<std::mutex> lck(m_mutex);
        auto it = m_maps.begin();
        for(; it != m_maps.end(); ++it){
            ret[it->first] = it->second->getContent();
            it->second = nullptr; //help memory.
        }
        return ret;
    }
private:
    FuncCreateWriter m_func;
    std::mutex m_mutex;
    std::map<String, std::shared_ptr<IMemoryWriter>> m_maps;
};

}
