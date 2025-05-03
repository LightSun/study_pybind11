#pragma once

#include <string>
#include <vector>
#include "json/med_json.h"
#include "fastdeploy/vision/common/result.h"

//fastdeploy::vision::OCRResult
namespace fastdeploy {
namespace vision {
DEF_JSON_HEAD(OCRResult);
}
}

namespace ocr {

using String = std::string;

struct InputItem{
    String id;
    String base64;
};

struct Input{
    std::vector<String> ids;
    std::vector<String> base64Imgs;
};

struct Output{
    std::vector<String> ids;
    std::vector<fastdeploy::vision::OCRResult> results;
    String msg;
};

DEF_JSON_HEAD(InputItem);
DEF_JSON_HEAD(Input);
DEF_JSON_HEAD(Output);

}


