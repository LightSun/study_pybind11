#include <sstream>
#include <stdio.h>
#include "CmdBuilder2.h"
#include "msvc_compat.h"

#ifdef __linux
#include <unistd.h>
#endif

struct CharBuf{

    char* data {nullptr};

    CharBuf(int len){
        data = (char*)malloc(len);
    }
    CharBuf(int len, char val){
        data = (char*)malloc(len);
        memset(data, val, len);
    }
    ~CharBuf(){
        if(data){
            free(data);
            data = nullptr;
        }
    }
};
#define CMD_RESULT_BUF_SIZE 2048
#undef PRINTERR
#undef PRINTLN
#define PRINTERR(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__)
#define PRINTLN(fmt, ...) fprintf(stdout, fmt, ##__VA_ARGS__)

static bool chechCmdRet(int ret){
    if(1 == ret){
        //cancelled: cmdRunner
        PRINTERR("cmd is empty or be cancelled.\n");
    }
    else if(-1 == ret){
        PRINTERR("create sub process failed.\n");
    }
    else if(0x7f00 == ret){
        PRINTERR("cmd is error, can't run.\n");
    }
    else{
#ifdef _WIN32
        return ret == 0;
#else
        if(WIFEXITED(ret)){
            int s = WEXITSTATUS(ret);
            PRINTLN("normal exit：%d\n", s);
            return s == 0;
        }
        else if(WIFSIGNALED(ret)){
            PRINTERR("killed by signal， signal is：%d\n", WTERMSIG(ret));
        }
        else if(WIFSTOPPED(ret)){
            PRINTERR("stpped by signal，signal is ：%d\n", WSTOPSIG(ret));
        }else{
            return true;
        }
#endif
    }
    return false;
}

bool med_qa::CmdHelper2::runCmdDirectly(String& ret){
    CharBuf cbuf(CMD_RESULT_BUF_SIZE, 0);
    char* buf_ps = cbuf.data;
    std::stringstream ss;

    FILE *fptr;

    int c;
    if((fptr = popen(m_cmd.data(), "r")) != NULL)
    {
#ifdef _WIN32
        while((c = fread(buf_ps, 1, CMD_RESULT_BUF_SIZE, fptr)) > 0){
            if(isReqStop()){
                break;
            }
            ss << String(buf_ps, c);
        }
//        while (fgets(buf_ps, CMD_RESULT_BUF_SIZE, fptr) != NULL) {
//            if(isReqStop()){
//                break;
//            }
//            ss << String(buf_ps);
//        }
#else
        while((c = read(fileno(fptr), buf_ps, CMD_RESULT_BUF_SIZE)) > 0){
            if(isReqStop()){
                break;
            }
            ss << String(buf_ps, c);
        }
#endif
        int _ret = pclose(fptr);
        fptr = NULL;
        if(isReqStop()){
            return false;
        }
        ret = ss.str();
        return chechCmdRet(_ret);
    }
    return false;
}
