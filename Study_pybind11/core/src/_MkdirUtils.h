#pragma once

#ifdef WIN32
#include <io.h>
#include <direct.h>
#else
#include <unistd.h>
#include <sys/stat.h>
#endif
#include <stdint.h>
#include <string>

#include <list>
#include "core/src/FileUtils.h"

#ifdef WIN32
#define ACCESS(fileName,accessMode) _access(fileName,accessMode)
#define MKDIR(path) _mkdir(path)
#else
#define ACCESS(fileName,accessMode) access(fileName,accessMode)
#define MKDIR(path) mkdir(path,S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)
#endif

namespace h7 {

static inline int32_t createDirectory(const std::string &directoryPath)
{
    if(FileUtils::isFileExists(directoryPath)){
        return 0;
    }
    //find the last exist dir.
    std::list<String> dirs;
    dirs.push_front(directoryPath);
    String last_dir = directoryPath;
    while (true) {
        String dir = FileUtils::getFileDir(last_dir);
        if(FileUtils::isFileExists(dir)){
            break;
        }
        dirs.push_front(dir);
        last_dir = dir;
    }

    auto it = dirs.begin();
    for(;it != dirs.end(); ++it){
        String& str = *it;
        MKDIR(str.data());
    }
    return 0;
}
}

