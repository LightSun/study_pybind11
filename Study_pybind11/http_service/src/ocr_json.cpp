#include "ocr_json.h"

namespace ocr {
namespace json_impl {
void to_json(nlohmann::json& j, const InputItem& p){
    PP_JSON_TO(id, base64);
}
void from_json(const nlohmann::json& j, InputItem& p){
    PP_JSON_FROM(id, base64);
}

void to_json(nlohmann::json& j, const Input& p){
    PP_JSON_TO(ids, base64Imgs);
}
void from_json(const nlohmann::json& j, Input& p){
    PP_JSON_FROM(ids, base64Imgs);
}
void to_json(nlohmann::json& j, const Output& p){
    PP_JSON_TO(ids, results, msg);
}
void from_json(const nlohmann::json& j, Output& p){
    PP_JSON_FROM(ids, results, msg);
}
}
}

//---------------
namespace fastdeploy {
namespace vision {
namespace json_impl {
void to_json(nlohmann::json& j, const OCRResult& p){
    PP_JSON_TO(boxes, text, rec_scores, cls_scores, cls_labels);
}
void from_json(const nlohmann::json& j, OCRResult& p){
    PP_JSON_FROM(boxes, text, rec_scores, cls_scores, cls_labels);
}
}}
}

