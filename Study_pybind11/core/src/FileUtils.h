#pragma once

#include <stdio.h>
#include <cstdio>
#include <initializer_list>
#include "core/src/common.h"
#include "core/src/c_common.h"

namespace h7 {
    class FileUtils{
    public:
        static bool isFileExists(CString path);
        static bool isDirReadable(CString dir);

        static inline bool deleteFile(CString file){
            return remove(file.c_str()) == 0;
        }
        static String getCurrentDir();
        static bool isRelativePath(CString path);
        static String getFilePath(CString path, const std::vector<String>& dirs);

        static std::vector<String> getFiles(CString path, bool recursion = true,
                                     CString suffix = "");
        static std::vector<String> getFilesContains(CString path, bool recursion,
                                     CString word);
        static std::vector<String> getFileDirs(CString path);
        //remove dir and below-files
        static bool removeDirectory(CString path);

        static inline String getFileDir(CString file){
            int pos;
#if defined(_WIN32) || defined(WIN32)
            pos = file.rfind("/");
            if(pos < 0){
                pos = file.rfind("\\");
            }
#else
            pos = file.rfind("/");
#endif
            return file.substr(0, pos);
        }
        static inline String getFileName(CString file){
            int index = file.rfind("/");
            if(index < 0){
                index = file.rfind("\\");
            }
            int dot_i = file.rfind(".");
            if(dot_i < 0){
                return file.substr(index + 1);
            }
            int len = dot_i - index - 1;
            if(len < 0){
                return file.substr(index + 1);
            }
            MED_ASSERT(len > 0);
            return file.substr(index + 1, len);
        }

        static inline String getSimpleFileName(CString file){
            int index = file.rfind("/");
            if(index < 0){
                index = file.rfind("\\");
            }
            MED_ASSERT(index >= 0);
            return file.substr(index + 1);
        }

        static int getFileLineCount(CString file);
        static void mkdir(CString path);
        static void mkdirs(CString path);

        static unsigned int hash(CString file);
        static bool writeFile(CString file, CString content);
        static std::string getFileContent(CString file);
        static std::string getFileContent(CString file, uint64 offset, uint64 size);
        static uint64 getFileSize(CString file);
        static std::vector<String> readLines(CString file);
    };
}
