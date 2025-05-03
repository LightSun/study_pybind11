#pragma once

#include <string>
#include <vector>
#include <map>

namespace h7 {
    class CacheManager;
}

namespace med_qa {

using CString = const std::string &;
using String = std::string;
template<typename T>
using List = std::vector<T>;
template<typename T>
using CList = const std::vector<T>&;

class EDManager
{
public:
    //enc
    EDManager(CString encFileDirs, CString encFileSuffixes, CString encOutDesc):
        m_encFileDirs(encFileDirs),m_encFileSuffixes(encFileSuffixes),
        m_encOutDesc(encOutDesc){
        if(m_encOutDesc.find(",") == String::npos){
            m_encOutDesc = m_encOutDesc + ",record,data";
        }
    }
    //for dec
    EDManager() = default;
    ~EDManager();
    //, for multi
    void setIgnorePath(CString paths);
    //, for multi
    void setIncludePath(CString paths);
    List<String>& getKeys(){return m_keys;}
    List<String>& getInputShapeStrs(){return m_inputShapes;}
    List<String>& getInputNames(){return m_inputNames;}

    void compress(bool verify);
    void compress(CList<String> files, CList<String> keys,bool verify);

    void load(CString encOutDesc);
    void loadDir(CString dir);
    String getItem(CString key);
    void addItem(CString key, CString data);
    void removeItem(CString key);
    void removeItemIfExclude(CList<String> keys);

    //merge the manager, if have same item. retain current's.
    EDManager* merge(EDManager& oth);

    EDManager* mergeByPriority(EDManager& oth, bool curIsHigh);
    //often used after merge.
    void compressTo(CString encOutDesc);

    void compressTo(CString encOutDesc, CString salt);
public:
    void prints(int limitLen = 1024);
    void checkKPID();

    static String getKPID();
private:
    bool shouldIgnore(CString path);
    bool shouldInclude(CString path);

private:
    List<String> m_keys;
    List<String> m_inputNames;
    List<String> m_inputShapes;
    String m_encFileDirs;
    String m_encFileSuffixes;
    String m_encOutDesc;
    List<String> m_ignorePath;//dirs or files
    List<String> m_incPaths;
    h7::CacheManager* m_cacheM {nullptr};
};

}
