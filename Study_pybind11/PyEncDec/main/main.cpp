
#include <iostream>
#include <fstream>
#ifndef _WIN32
#include <sys/io.h>
#include <sys/stat.h>
#endif

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include "AES.h"
#include "gzip/src/Gzip.h"
#include "../com/com.h"

using CString = const std::string&;
using String = std::string;

static bool isDir0(CString dir);
static inline std::vector<String> buildArgs();
static inline std::vector<String> buildArgs_dir();

int main_file(int argc, const char* argv[]);
int main_dir(int argc, const char* argv[]);

int main(int argc, const char* argv[]){
    if(argc == 1){
        //auto args = buildArgs();
        auto args = buildArgs_dir();
        //
        std::vector<const char*> argvs;
        argvs.resize(args.size());
        for(int i = 0 ; i < (int)args.size() ; i ++){
            argvs[i] = args[i].c_str();
        }
        return main(args.size(), argvs.data());
    }
    //xx <in_file> <out_file>
    if(argc < 3){
        fprintf(stderr, "xx <in_file> <out_file>\n");
        return 1;
    }
    String inFile = argv[1];
    if(isDir0(inFile)){
        return main_dir(argc, argv);
    }else{
        return main_file(argc, argv);
    }
}

std::vector<String> buildArgs(){
    String mdDir = "/media/heaven7/Elements_SE/study/work/TODO/onnx_desc";
    String inFile = mdDir + "/dataX2.zip";
    String outFile = mdDir + "/dataX2.zip.enc";
    std::vector<String> args = {
        "modelM",
        inFile,
        outFile,
        "1G"
    };
    return args;
}

std::vector<String> buildArgs_dir(){
    String mdDir = "/media/heaven7/Elements_SE/study/work/TODO";
    String inFile = mdDir + "/onnx_desc";
    String outFile = mdDir + "/onnx_desc.enc";
    std::vector<String> args = {
        "modelM",
        inFile,
        outFile,
    };
    return args;
}

//----------------------------
bool isDir0(CString path){
    struct stat         infos;

    if (stat(path.data(), &infos) != 0){
        return false;    //无效
    }
    else if (infos.st_mode & S_IFDIR){
        return true;    //目录
    }
    else if (infos.st_mode & S_IFREG){
        return false;    //文件
    }
    else{
        return false;
    }
}
int main_file(int argc, const char* argv[]){
    String inFile = argv[1];
    String outFile = argv[2];
    size_t blockSize = 2 << 20;
    if(argc >= 4){
        String sizeDesc = argv[3];
        String sizeStr = sizeDesc.substr(0, sizeDesc.length()-1);
        if(sizeDesc.length() >= 2){
            size_t size = std::stoul(sizeStr);
            char lastChar = sizeDesc.data()[sizeDesc.size()-1];
            if(lastChar == 'M' || lastChar == 'm'){
                blockSize = size << 20;
            }else if(lastChar == 'G' || lastChar == 'g'){
                blockSize = size << 30;
            }else{
                fprintf(stderr, "wrong sizeDesc: '%s'\n", sizeDesc.data());
            }
        }

    }
    auto cs = EncDecPy::getFileContent0(inFile);
    if(cs.empty()){
        fprintf(stderr, "open input failed: %s\n", inFile.data());
        return 1;
    }
    String key1 = "@(**^$$%&)((&^^(";
    String key2 = "8g77d7fg7&%%(*^^";
    String content;
    //getFileContent0(arg)
    {
        med_ed::AES aes(8, blockSize);
        content = aes.cfb128(cs, key1, key2, true);
    }
    if(content.empty()){
        fprintf(stderr, "enc failed, infile = %s\n", inFile.data());
        return 1;
    }
    std::ofstream fos;
    fos.open(outFile.data(), std::ios::binary);
    if(!fos.is_open()){
        fprintf(stderr, "open output failed: %s\n", outFile.data());
        return 1;
    }
    fos.write(content.data(), content.length());
    return 0;
}
int main_dir(int, const char* argv[]){
    String inDir = argv[1];
    String outFile = argv[2];
    h7_gz::GzipHelper gh;
    //gh.setConcurrentThreadCount(2);
    gh.setUseSimpleEncDec();
    if(gh.compressDir(inDir, outFile)){
        return 0;
    }
    fprintf(stderr, "main_dir >> dec failed, inDir = '%s'.", inDir.data());
    return 1;
}
