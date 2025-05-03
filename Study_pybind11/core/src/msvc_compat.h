#pragma once

#ifdef _MSC_VER
#include <windows.h>
#define fopen64 fopen
#define fseeko64 _fseeki64
#define ftello64 _ftelli64
#define popen _popen
#define pclose _pclose
#endif
