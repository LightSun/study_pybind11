#pragma once

#include <map>
#include "core/src/Value.hpp"
#include "core/src/IKeyValue.h"

namespace h7 {
class Properties;
}

namespace med_qa {

using String = std::string;
using CString = const String&;

typedef struct _PropCtx _PropCtx;

class Prop: public h7::IKeyValue
{
public:
    Prop();
    ~Prop();

    void prints();
    void load(int argc, const char* argv[]);
    void load(CString propFile);
    h7::Properties* getRawProp();
    void copyFrom(std::map<String,String>& src);

    void putString(CString key, CString val) override;
    String getString(CString key, CString def = "") override;
    void print(CString prefix) override;
    std::set<String> keys() override;

    h7::Value getValue(CString key, CString def = ""){
        return h7::Value(getString(key, def));
    }
private:
    _PropCtx* m_ctx {nullptr};
};

}

