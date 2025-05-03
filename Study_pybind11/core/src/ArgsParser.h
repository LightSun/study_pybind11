#pragma once

#include <vector>
#include <string>
#include "core/src/Properties.hpp"

namespace h7 {
    class ArgsParser{
    public:
        using String = std::string;
        using CString = const std::string&;
        typedef int (*Func_Main)(int argc, char* argv[]);

        ArgsParser(const std::vector<String>& vec):m_args(vec){}
        ArgsParser(int argc,char* args[]){
            m_args.reserve(argc - 1);
            for(int i = 1 ; i < argc ; ++i){
                m_args.push_back(args[i]);
            }
        }

        h7::Properties asProperty(){
            h7::Properties prop;
            for(int i = 0 ; i < m_args.size() ; ++i){
                String& val = m_args[i];
                if(val.find("---") == 0){
                    String name = val.substr(3);
                    std::vector<String> args;
                    for(int ti = i + 1; ; ++ ti){
                        if(val.find("-") == 0){
                            break;
                        }
                        args.push_back(m_args[ti]);
                    }
                    i += args.size();
                    // handle value
                    String val;
                    for(int k = 0 ; k < args.size() ; ++k){
                        val += args[k];
                        if(k != args.size() - 1){
                            val += ",";
                        }
                    }
                    prop.setProperty(name, val);
                }
                else if(val.find("--") == 0){
                    prop.setProperty(val.substr(2), m_args[i + 1]);
                    ++i;
                }else if(val.find("-") == 0){
                    prop.setProperty(val.substr(1), "1");//indicate true
                }
            }
            return prop;
        }
        int run(Func_Main ptr){
            char** param_ptrs = (char**)malloc(m_args.size() * sizeof (char*));
            int c = m_args.size();
            for(int i = 0 ; i < c ; ++i){
               param_ptrs[i] = (char*)m_args[i].data();
            }
            int ret = ptr(c, param_ptrs);
            free(param_ptrs);
            return ret;
        }
    private:
        std::vector<String> m_args;
    };
}
