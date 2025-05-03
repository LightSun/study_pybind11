#pragma once

#include <string>
#include <functional>
#include <vector>

namespace med_qa {
class Prop;
}

namespace med_http {

using String = std::string;
using CString = const std::string&;

struct HttpReqItem{
    String reqPath;
    bool isGet {false};
    std::function<String(CString)> func_processor;
};

void startHttpService(int port, int tc,const std::vector<HttpReqItem>& items);

void startService(med_qa::Prop* p);

}
