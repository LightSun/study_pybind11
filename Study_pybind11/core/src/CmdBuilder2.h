#pragma once

#include <string.h>
#include <stdio.h>
#include "core/src/h_atomic.h"
#include "core/src/string_utils.hpp"

namespace med_qa {
using CString = const std::string&;
using String = std::string;
    class CmdBuilder2{
    public:
        using SelfR = CmdBuilder2&;
        inline SelfR str(CString str){
            mCmds.push_back(str);
            return *this;
        }
        inline SelfR strs(CString str){
            auto l = h7::utils::split(" ", str);
            return strs(l);
        }
        inline SelfR strs(std::vector<String>& oth){
            mCmds.insert(mCmds.end(), oth.begin(), oth.end());
            return *this;
        }
        inline SelfR strs(const std::vector<String>& oth){
            mCmds.insert(mCmds.end(), oth.begin(), oth.end());
            return *this;
        }
        template <typename ...Args>
        inline SelfR strFmt(const char* fmt, Args&& ... args){
            HFMT_BUF_256({
                         mCmds.push_back(buf);
                         }, fmt, std::forward<Args>(args)...);
            return *this;
        }
        template <typename ...Args>
        inline SelfR strFmtN(int bufLen, const char* fmt, Args&& ... args){
            MED_ASSERT(bufLen >= (int)strlen(fmt) + 1);
            char buf[bufLen];
            snprintf(buf, bufLen, fmt, std::forward<Args>(args)...);
            return str(buf);
        }
        template <typename ...Args>
        inline SelfR strsFmt(const char* fmt, Args&& ... args){
            HFMT_BUF_256({
                         strs(buf);
                         }, fmt, std::forward<Args>(args)...);
            return *this;
        }
        template <typename ...Args>
        inline SelfR strsFmtN(int bufLen,const std::string& fmt, Args&& ... args){
            MED_ASSERT(bufLen >= (int)fmt.length());
            char buf[bufLen];
            int n = snprintf(buf, bufLen, fmt.data(), std::forward<Args>(args)...);
            //MED_ASSERT(n >= 0 && n <= bufLen);
            if(n < 0 || n > bufLen){
                fprintf(stderr, "strsFmtN.snprintf return wrong.bufLen = %d, "
                                "but n = %d\n", bufLen, n);
            }
            return strs(String(buf, n));
        }
        inline SelfR and0(){
            mCmds.push_back("&&");
            return *this;
        }
        inline SelfR or0(){
            mCmds.push_back("|");
            return *this;
        }
        inline std::vector<String>& toCmd(){
            return mCmds;
        }
        inline SelfR clear(){
            mCmds.clear();
            return *this;
        }
        inline String toCmdStr(){
            int size = mCmds.size();
            String str;
            str.reserve(512);
            for(int i = 0 ; i < size ; ++i){
                str += mCmds[i];
                if(i != size - 1){
                    str += " ";
                }
            }
            return str;
        }
    private:
        std::vector<String> mCmds;
    };

    class CmdHelper2{
    public:
        CmdHelper2(CString cmd):m_cmd(cmd){}
        CmdHelper2(const std::vector<String>& cmds){
            int size = cmds.size();
            String str;
            str.reserve(512);
            for(int i = 0 ; i < size ; ++i){
                str += cmds[i];
                if(i != size - 1){
                    str += " ";
                }
            }
            m_cmd = str;
        }
        bool runCmdDirectly(String& ret);

        void stop(){
            h_atomic_cas(&m_reqStop, 0, 1);
        }
        bool isReqStop(){
            return h_atomic_get(&m_reqStop) == 1;
        }
        String& getCmd(){return m_cmd;}

    private:
        String m_cmd;
        volatile int m_reqStop {0};
    };
}
