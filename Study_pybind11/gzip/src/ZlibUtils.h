#pragma once

#include <vector>
#include <string>

namespace h7_gz {

using CString = const std::string&;
using String = std::string;

struct IZlibInput{

    virtual bool hasNext() = 0;

    //isFinish: true means no more data.
    //return valid len of buffer.
    virtual size_t next(std::vector<char>& vec,bool& isFinish) = 0;
};

struct IZlibOutput{

    virtual bool write(std::vector<char>& buf, size_t len) = 0;
};

class ZlibUtils
{
public:
    static bool compress(IZlibInput* in, IZlibOutput* out);

    static bool decompress(IZlibInput* in, IZlibOutput* out);

    static bool compress(CString str, String& out);

    static bool decompress(CString str, String& out);
};

}

