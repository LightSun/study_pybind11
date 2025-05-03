#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <fstream>

#ifdef _WIN32
#include <direct.h>
#include <io.h>
#else
#include <sys/stat.h>
#endif

#ifdef __APPLE__
#include <sys/uio.h>
#elif _WIN32
#include <io.h>
#elif __linux__
#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <error.h>
//#include <sys/stat.h>
#else
#include <sys/io.h>
#endif

#ifdef WIN32
#include <windows.h>                 //Windows API   FindFirstFile
#include <shlwapi.h>
#undef CString
#endif

#include <list>
#include "FileUtils.h"
#include "string_utils.hpp"
#include "hash.h"
#include "StringBuffer.h"
//#include "openssl/sha.h"
#include "FileIO.h"
#include "_MkdirUtils.h"

//windows: https://blog.csdn.net/siyacaodeai/article/details/112732678
//linux: https://www.jianshu.com/p/7f79d496b0e2
namespace h7 {

bool FileUtils::isFileExists(CString path) {
#ifdef _WIN32
    WIN32_FIND_DATA wfd;
    HANDLE hFind = FindFirstFile(path.data(), &wfd);
    if ( INVALID_HANDLE_VALUE != hFind )  return true;
    else                                  return false;
#else
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
#endif // !_WIN32
}

bool FileUtils::isDirReadable(CString dir){
    int code;
#ifdef _WIN32
    //00, 02, 04, 06
    code = _access(dir.data(), 4);
    return code == 0;
#else
    code = access(dir.data(), R_OK);
    return code == 0;
#endif
}

std::vector<String> FileUtils::getFiles(CString path, bool recursion,
                                        CString suffix){
    std::vector<String> ret;
#ifdef __linux__
    DIR *dir;
#endif
#ifdef _WIN32
    long hFile = 0;
    struct _finddata_t fileinfo;
    std::string p;
#endif
    struct dirent *ptr;
    std::list<String> folders;
    folders.push_back(path);

    String str;
    while(folders.size() > 0) {
      str = folders.front();
      folders.pop_front();
#ifdef __linux__
      //printf("readable:%s = %d\n", str.data(), isDirReadable(str));
      if ((dir = opendir(str.c_str())) != NULL) {
          while ((ptr = readdir(dir)) != NULL) {
              //printf("d_name:%s, d_type = %d\n", ptr->d_name, ptr->d_type);
              if(strcmp(ptr->d_name, ".") == 0 || strcmp(ptr->d_name, "..") == 0) {
                  continue;
              } else if (ptr->d_type == DT_REG) { /* regular file */
                  String _file(ptr->d_name);
                  //printf("d_name: '%s'\n", _file.data());
                  if(suffix.length() > 0){
                      if(h7::utils::endsWith(_file, suffix)){
                          ret.push_back(str + std::string("/") + _file);
                      }
                  }else{
                      ret.push_back(str + std::string("/") + _file);
                  }
              } else if (ptr->d_type == DT_DIR) { /* dir */
                  if(recursion){
                      folders.push_back(str + std::string("/") + ptr->d_name);
                  }
              }
          }
      }else{
          fprintf(stderr, "read dir: failed. '%s'\n", str.data());
      }
      closedir(dir);
#else
      fprintf(stderr, "non-linux\n");
#endif

#ifdef _WIN32
    if ((hFile = _findfirst(p.assign(path).append("\\*").c_str(), &fileinfo)) != -1)
    {
        do{
            if ((fileinfo.attrib & _A_SUBDIR)){
                if (strcmp(fileinfo.name, ".") != 0 && strcmp(fileinfo.name, "..") != 0)
                {
                    if(recursion){
                        folders.push_back(p.assign(path).append("\\").append(fileinfo.name));
                    }
                }
            }
            else{
                String _file(fileinfo.name);
                if(suffix.empty() || h7::utils::endsWith(_file, suffix)){
                    ret.push_back(p.assign(path).append("\\").append(fileinfo.name));
                }
            }
        }while (_findnext(hFile, &fileinfo) == 0);
        _findclose(hFile);
    }
#endif
    }
    return ret;
}

std::vector<String> FileUtils::getFilesContains(CString path, bool recursion,
                                     CString word){
    MED_ASSERT(!word.empty());
    std::vector<String> ret;
#ifdef __linux__
    DIR *dir;
#endif
#ifdef _WIN32
    long hFile = 0;
    struct _finddata_t fileinfo;
    std::string p;
#endif
    struct dirent *ptr;
    std::list<String> folders;
    folders.push_back(path);

    String str;
    while(folders.size() > 0) {
      str = folders.front();
      folders.pop_front();
#ifdef __linux__
      if ((dir = opendir(str.c_str())) != NULL) {
          while ((ptr = readdir(dir)) != NULL) {
              if(strcmp(ptr->d_name, ".") == 0 || strcmp(ptr->d_name, "..") == 0) {
                  continue;
              } else if (ptr->d_type == DT_REG) { /* regular file */
                  String _file(ptr->d_name);
                  if(_file.find(word) != String::npos){
                      ret.push_back(str + std::string("/") + _file);
                  }
              } else if (ptr->d_type == DT_DIR) { /* dir */
                  if(recursion){
                      folders.push_back(str + std::string("/") + ptr->d_name);
                  }
              }
          }
      }
      closedir(dir);
#endif

#ifdef _WIN32
    if ((hFile = _findfirst(p.assign(path).append("\\*").c_str(), &fileinfo)) != -1)
    {
        do{
            if ((fileinfo.attrib & _A_SUBDIR)){
                if (strcmp(fileinfo.name, ".") != 0 && strcmp(fileinfo.name, "..") != 0)
                {
                    if(recursion){
                        folders.push_back(p.assign(path).append("\\").append(fileinfo.name));
                    }
                }
            }
            else{
                String _file(fileinfo.name);
                if(_file.find(word) != String::npos){
                    ret.push_back(p.assign(path).append("\\").append(fileinfo.name));
                }
            }
        }while (_findnext(hFile, &fileinfo) == 0);
        _findclose(hFile);
    }
#endif
   }
   return ret;
}
std::vector<String> FileUtils::getFileDirs(CString path){
    std::vector<String> ret;
#ifdef __linux__
    DIR *dir;
#endif
#ifdef _WIN32
    long hFile = 0;
    struct _finddata_t fileinfo;
    std::string p;
#endif
    struct dirent *ptr;
    std::list<String> folders;
    folders.push_back(path);

    String str;
    while(folders.size() > 0) {
      str = folders.front();
      folders.pop_front();
#ifdef __linux__
      if ((dir = opendir(str.c_str())) != NULL) {
          while ((ptr = readdir(dir)) != NULL) {
              if(strcmp(ptr->d_name, ".") == 0 || strcmp(ptr->d_name, "..") == 0) {
                  continue;
              } else if (ptr->d_type == DT_REG) { /* regular file */
                  //ignore
              } else if (ptr->d_type == DT_DIR) { /* dir */
                  ret.push_back(str + std::string("/") + ptr->d_name);
                  //next iterate
                  folders.push_back(str + std::string("/") + ptr->d_name);
              }
          }
      }
      closedir(dir);
#endif

#ifdef _WIN32
    if ((hFile = _findfirst(p.assign(path).append("\\*").c_str(), &fileinfo)) != -1)
    {
        do{
            if ((fileinfo.attrib & _A_SUBDIR)){
                if (strcmp(fileinfo.name, ".") != 0 && strcmp(fileinfo.name, "..") != 0)
                {
                    if(recursion){
                        folders.push_back(p.assign(path).append("\\").append(fileinfo.name));
                        ret.push_back(p.assign(path).append("\\").append(fileinfo.name));
                    }
                }
            }
            else{
                //file. ignore
            }
        }while (_findnext(hFile, &fileinfo) == 0);
        _findclose(hFile);
    }
#endif
    }
    return ret;
}

bool FileUtils::removeDirectory(CString path) {
#ifdef __linux__
    DIR *dir = opendir(path.data());
    if (dir == nullptr) {
        return false;
    }

    dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        std::string fullPath = std::string(path) + "/" + entry->d_name;

        if (entry->d_type == DT_DIR) {
            if (!removeDirectory(fullPath.c_str())) {
                closedir(dir);
                return false;
            }
        } else {
            if (remove(fullPath.c_str()) != 0) {
                closedir(dir);
                return false;
            }
        }
    }
    closedir(dir);
    if (rmdir(path.data()) != 0) {
        return false;
    }
    return true;
#endif
    fprintf(stderr, "removeDirectory >> non-linux, not support now\n");
    return false;
}

#ifdef _WIN32
static char ALPHABET[] = {
    'A','B','C','D','E','F','G',
    'H','I','J','K','L','M','N',
    'O','P','Q','R','S','T',
    'U','V','W','X','Y','Z'
};
static void Wchar_tToString(std::string& szDst, wchar_t*wchar)
{
    wchar_t * wText = wchar;
    DWORD dwNum = WideCharToMultiByte(CP_OEMCP,NULL,wText,-1,NULL,0,NULL,FALSE);
    char *psText;  // psText为char*的临时数组，作为赋值给std::string的中间变量
    psText = new char[dwNum];
    WideCharToMultiByte(CP_OEMCP,NULL,wText,-1,psText,dwNum,NULL,FALSE);
    szDst = psText;
    delete []psText;
}
#endif

String FileUtils::getCurrentDir(){
#ifdef _WIN32
    //WCHAR czFileName[1024] = {0};
    //String ret;
    CHAR czFileName[1024] = {0};
    GetModuleFileName(NULL, czFileName, _countof(czFileName)-1);
    PathRemoveFileSpec(czFileName);
    //Wchar_tToString(ret, czFileName);
    return String(czFileName);
   // fprintf(stderr, "getCurrentDir >> win32 not support\n");
#else
    char buf[1024];
    getcwd(buf, 1024);
    return String(buf);
#endif
}
bool FileUtils::isRelativePath(CString path){
    auto strs = path.c_str();
    if(strs[0] == '/'){
        return false;
    }
#ifdef _WIN32
    if(strs[1]==':'){
        for(int i = 0 ; i < 26 ; i ++){
            if(ALPHABET[i] == strs[0]){
                return false;
            }
        }
    }
#endif
    return true;
}

String FileUtils::getFilePath(CString path, const std::vector<String>& dirs){
    if(!isRelativePath(path)){
       return path;
    }
    if(isFileExists(path)){
        return path;
    }
    String file;
    auto end = dirs.end();
    for(auto it = dirs.begin() ; it != end ; it++){
         file = *it + "/" + path;
         if(isFileExists(file)){
             return file;
         }
    }
    return "";
}

int FileUtils::getFileLineCount(CString file){
    FILE *fp;
    if((fp = fopen(file.c_str(), "r")) == NULL){
        return -1;
    }
    int flag;
    int count = 0;
    while(!feof(fp))
    {
        flag = fgetc(fp);
        if(flag == '\n'){
           count++;
        }
    }
    fclose(fp);
    return count + 1; //add last line
}

void FileUtils::mkdir(CString path){
#ifdef _WIN32
  _mkdir(path.c_str());
#else
  ::mkdir(path.c_str(), 0777);
#endif // !_WIN32
}

void FileUtils::mkdirs(CString path){
    h7::createDirectory(path);
}

//ifstream
std::string FileUtils::getFileContent(CString file){
    FileInput fin(file);
    MED_ASSERT_X(fin.is_open(), "open file failed: " + file);
    std::vector<char> buf(fin.getLength());
    fin.reset();
    fin.read(buf.data(), buf.size());
    fin.close();
    return std::string(buf.data(), buf.size());
}

bool FileUtils::writeFile(CString file, CString content){
    auto dir = getFileDir(file);
    mkdirs(dir);
    std::ofstream fos(file, std::ios::binary);
    if(fos.is_open()){
         fos.write(content.data(), content.length());
         fos.close();
         return true;
    }
    return false;
}

std::string FileUtils::getFileContent(CString file, uint64 offset, uint64 size){
    FileInput fin(file);
    MED_ASSERT_X(fin.is_open(), "open file failed: " + file);
    uint64 left_size = fin.getLength() - offset;
    MED_ASSERT(left_size > 0);
    //
    std::vector<char> buf(HMIN(left_size, size));
    fin.seek(offset, false);
    fin.read(buf.data(), buf.size());
    fin.close();
    return std::string(buf.data(), buf.size());
}

uint64 FileUtils::getFileSize(CString file){
    FileInput fin(file);
    MED_ASSERT_X(fin.is_open(), "open file failed: " + file);
    return fin.getLength();
}

unsigned int FileUtils::hash(CString file){
    FileInput fin(file);
    MED_ASSERT_X(fin.is_open(), "open file failed: " + file);
    std::vector<char> buf(fin.getLength());
    fin.reset();
    fin.read(buf.data(), buf.size());
    fin.close();
    return fasthash32(buf.data(), buf.size(), 11);
}
std::vector<String> FileUtils::readLines(CString file){
    std::vector<String> vec;
    auto buffer = getFileContent(file);
    h7::StringBuffer sb(buffer);
    String str;
    while (sb.readLine(str)) {
        vec.push_back(str);
    }
    return vec;
}

}


