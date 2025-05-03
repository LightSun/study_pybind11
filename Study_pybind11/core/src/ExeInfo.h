#pragma once

#include "core/src/common.h"
#include "core/src/IKeyValue.h"
#include "core/src/FileUtils.h"
#include "core/src/Prop.h"
#include "core/src/collections.h"
#include "core/src/CmdBuilder2.h"

#define _RT_LIB_SEP ":"

namespace h7_app {

using String = std::string;
using CString = const std::string&;
template<typename T>
using List = std::vector<T>;

struct ExeInfo{
    String binExe;
    String name; //without suffix.

    List<String> prefixParams;
    List<String> suffixParams;
    List<String> ldPaths;

    bool parse(CString rootDir, h7::KVHelper& h){
        auto targetName = h.getString("MainExe");
        MED_ASSERT_X(!targetName.empty(), "must assign MainExe.");
        auto tName = h7::FileUtils::getFileName(targetName);
        const String binExe = rootDir + "/main/bin/" + targetName;
        if(!h7::FileUtils::isFileExists(binExe)){
            MED_ASSERT_X(false, "file not exist. " << binExe);
            return false;
        }
        this->binExe = binExe;
        this->name = tName;
        return true;
    }

    void parseRuntimeEnv(CString mainDir, bool needSuffixPs){
        String runtimeEnvFile = mainDir + "/gen/runtime.env";
        if(h7::FileUtils::isFileExists(runtimeEnvFile)){
            auto lds = h7::FileUtils::getFileContent(runtimeEnvFile);
            if(!lds.empty()){
                auto paths = h7::utils::split(",", lds);
                ldPaths.insert(ldPaths.end(),
                                   paths.begin(), paths.end());
            }
        }else{
            printf("[Warn] gen/runtime.env not exist.\n");
        }
        auto tName = h7::FileUtils::getFileName(binExe);
        const String runParamFile = mainDir + "/run/" + tName + ".param";
        if(h7::FileUtils::isFileExists(runParamFile)){
            med_qa::Prop prop;
            prop.load(runParamFile);
            h7::KVHelper helper(&prop);
            helper.getVector("PrefixParams", prefixParams);
            if(needSuffixPs){
                helper.getVector("SuffixParams", suffixParams);
            }
            auto lds = helper.getString("LD_LIBRARY_PATH");
            if(!lds.empty()){
                auto paths = h7::utils::split(",", lds);
                ldPaths.insert(ldPaths.end(), paths.begin(),
                                   paths.end());
            }
        }
    }
    void applyLdPaths(){
        auto fd_libs = med_qa::concatStr(ldPaths, _RT_LIB_SEP);
        setLD_LIBRARY_PATH(fd_libs);
    }
    void parseAndRun(CString mainDir, bool parseSuffixPs, bool autoRestart){
        parseRuntimeEnv(mainDir, parseSuffixPs);
        applyLdPaths();
        List<String> rtParams;
        for(int i = 0 ; i < (int)prefixParams.size() ; ++i){
            rtParams.push_back(prefixParams[i]);
        }
        rtParams.push_back(binExe);
        for(int i = 0 ; i < (int)suffixParams.size() ; ++i){
            rtParams.push_back(suffixParams[i]);
        }
        runInternal(rtParams, autoRestart);
    }
    //split with ','
    static void setLD_LIBRARY_PATH2(CString lds){
        if(lds.empty()){
            return;
        }
        auto ldPaths = h7::utils::split(",", lds);
#ifdef _WIN32
        MED_ASSERT_X(false, "windows not ok, now.");
#else
        auto rt_libs = med_qa::concatStr(ldPaths, ":");
        setLD_LIBRARY_PATH(rt_libs);
#endif
    }
    static void setLD_LIBRARY_PATH(CString fd_libs){
        if(fd_libs.empty()){
            return;
        }
        auto path = getenv("LD_LIBRARY_PATH");
        if(!path || strlen(path) == 0){
            setenv("LD_LIBRARY_PATH", fd_libs.data(), 1);
        }else{
            String apath = String(path) + _RT_LIB_SEP + fd_libs;
            setenv("LD_LIBRARY_PATH", apath.data(), 1);
        }
    }
    static void runInternal(const List<String>& rtParams,bool autoRestart){
        med_qa::CmdHelper2 ch(rtParams);
        auto cmd = ch.getCmd();
        do{
            if(system(cmd.data()) != 0){
                fprintf(stderr, "cmd failed: '%s'\n", cmd.data());
            }
        }while (autoRestart);
    }
};

}
