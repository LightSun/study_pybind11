#pragma once

#include "gzip/src/decode_mem.h"

namespace h7_gz {

//not used for std zip.
struct ZipFileItem
{
    String name;
    String shortName;//short name by name and dir
    String ext;      //extention of file
    size_t contentLen;
    String content;  //content, often used for memory file

    String readContent()const;

    static ZipFileItem ofMemoryFile(CString shortName,CString content);
};

struct GroupItem
{
    String name;
    std::vector<ZipFileItem> children;

    bool isEmpty()const{return children.empty();}
    String write(CString buffer) const;
    bool read(String& bufIn, String& bufOut);

    GroupItem filter(CString ext, bool remove);
};

typedef struct GzipHelper_Ctx0 GzipHelper_Ctx0;

class GzipHelper{
public:
    using FUNC_Classify = std::function<void(const std::vector<ZipFileItem>&, std::vector<GroupItem>&)>;
    //compressor to compress file content.
    using FUNC_Compressor = std::function<bool(const std::vector<ZipFileItem>&, String*)>;
    using FUNC_DeCompressor = std::function<bool(String&,std::vector<String>&)>;

    GzipHelper();
    ~GzipHelper();

public:
    void setUseSimpleEncDec();
    void setClassifier(FUNC_Classify func);
    void setCompressor(FUNC_Compressor func);
    void setDeCompressor(FUNC_DeCompressor func);
    void setConcurrentThreadCount(int count);
    void setAttentionFileExtensions(const std::vector<String>&);
    void setDebug(bool debug);
    void setExcludeDirs(const std::vector<String>&);

    bool compressDir(CString dir, CString outFile);
    bool compressFile(CString file, CString outFile);
    bool compressMemoryFiles(const std::vector<ZipFileItem>& items,
                             CString outFile);

    bool compressDirToMemory(CString dir, String& outData);
    bool compressFileToMemory(CString file, String& outData);
    bool compressMemoryFilesToMemory(const std::vector<ZipFileItem>& items,
                             String& outData);

    bool decompressFile(CString file, CString outDir);
    bool decompressFileToMemory(CString file, std::map<String,String>& out);

private:
    GzipHelper_Ctx0* m_ptr;
};

}
