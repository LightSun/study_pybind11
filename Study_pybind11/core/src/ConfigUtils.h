#pragma once

#include "core/src/common.h"
#include <map>

namespace h7 {

class ConfigUtils
{
public:
    static void loadPropertiesFromBuffer(CString buffer, std::map<String, String>& out);
    static void loadProperties(CString prop_file, std::map<String, String>& out);
    static void resolveProperties(const std::vector<String>& in_dirs,
                                  std::map<String, String>& out);
    static void resolveProperties(std::map<String, String>& out){
        std::vector<String> dirs;
        resolveProperties(dirs, out);
    }
};

}
