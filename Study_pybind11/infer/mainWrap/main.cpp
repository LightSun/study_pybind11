
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#else
#include "unistd.h"
#endif

using String = std::string;

static String getCurrentDir0();

void setRTENV(){
    String dir = "/media/heaven7/Elements_SE/study/work/OCR/compiled_fastdeploy_sdk";
    String FD_INSDIR = dir + "/third_libs/install";
    String LIB_TRT_DIR = FD_INSDIR + "/tensorrt/lib";
    String LIB_PADDLE_DIR = FD_INSDIR + "/paddle_inference/paddle/lib";
    String LIB_PAD2OX_DIR = FD_INSDIR + "/paddle2onnx/lib";
    String LIB_OPENVINO_DIR = FD_INSDIR + "/openvino/runtime/lib";
    String LIB_TOKEN_DIR = FD_INSDIR + "/fast_tokenizer/lib";
    String fd_libs = LIB_TRT_DIR + ":" + LIB_PADDLE_DIR;
    fd_libs += ":" + LIB_PAD2OX_DIR;
    fd_libs += ":" + LIB_OPENVINO_DIR;
    fd_libs += ":" + LIB_TOKEN_DIR;
    auto path = getenv("LD_LIBRARY_PATH");
    if(!path || strlen(path) == 0){
        setenv("LD_LIBRARY_PATH", fd_libs.data(), 1);
    }else{
        String apath = String(path) + ":" + fd_libs;
        setenv("LD_LIBRARY_PATH", apath.data(), 1);
    }
}

int main(int argc, const char* argv[]){
    setbuf(stdout, NULL);
    setRTENV();
    if(argc == 1){
        auto exe = getCurrentDir0() + "/OcrTest";
//        std::vector<String> args = {
//            "OcrTest"
//        };
//        const char* argvs[args.size()];
//        for(int i = 0 ; i < (int)args.size() ; i ++){
//            argvs[i] = args[i].c_str();
//        }
       // return main(args.size(), argvs);
        return system(exe.data());
    }
    return 0;
}

String getCurrentDir0(){
#ifdef _WIN32
    WCHAR czFileName[1024] = {0};
    GetModuleFileName(NULL, czFileName, _countof(czFileName)-1);
    PathRemoveFileSpec(czFileName);
    String ret;
    Wchar_tToString(ret, czFileName);
    return ret;
   // fprintf(stderr, "getCurrentDir >> win32 not support\n");
#else
    char buf[1024];
    getcwd(buf, 1024);
    return String(buf);
#endif
}
