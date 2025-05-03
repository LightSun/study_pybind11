#include <thread>

#include "core/src/base64.h"
#include "core/src/PerformanceHelper.h"
#include "core/src/Prop.h"
#include "IHttp.h"
#include "infer/src/fd_ocr.h"
#include "ocr_json.h"

using namespace nlohmann::literals;
using json = nlohmann::json;
static std::unique_ptr<h7::OcrApi> sOcrAPI;

namespace med_http {
static void _build_ocr(med_qa::Prop& p);
static void _do_start_service(med_qa::Prop& p);
}

void med_http::startService(med_qa::Prop* pr){
    auto& p = *pr;
    _build_ocr(p);
    _do_start_service(p);
}
void med_http::_build_ocr(med_qa::Prop& p){
    auto max_batch_cls = std::stoi(p.getString("max_batch_cls", "6"));
    auto max_batch_rec = std::stoi(p.getString("max_batch_rec", "8"));
    //
    h7::OcrConfig config;
    config.cache_dir = p.getString("cache_dir");
    config.model_desc_dir = p.getString("model_desc_dir");
    config.setRunMode(p.getString("run_mode"));
    config.max_batch_cls = max_batch_cls;
    config.max_batch_rec = max_batch_rec;
    //
    sOcrAPI = std::unique_ptr<h7::OcrApi>(new h7::OcrApi());
    auto& api = *sOcrAPI;
    api.setOcrConfig(config);
    MED_ASSERT(api.loadConfigs());
    MED_ASSERT(api.buildOCR());
}
void med_http::_do_start_service(med_qa::Prop& p){
   auto port = std::stoi(p.getString("port", "-1"));
   auto tc = std::stoi(p.getString("threadCnt", "-1"));
   MED_ASSERT(port >= 0);
   MED_ASSERT(tc >= 0);
   using namespace med_http;
   HttpReqItem reqItem;
   reqItem.reqPath = "/OcrInfer/infer";
   reqItem.isGet = false;
   reqItem.func_processor = [](const std::string& str){
       auto in = json::parse(str).get<ocr::Input>();
       ocr::Output out;
       out.ids = in.ids;
       if(in.base64Imgs.empty()){
           fprintf(stderr, "[Error] http: no image items\n");
           out.msg = "no image items";
       }
       else if(in.ids.size() != in.base64Imgs.size()){
           fprintf(stderr, "[Error] ids and base64Imgs: size should be the same.\n");
           out.msg = "ids and base64Imgs: size should be the same";
       }else{
           h7::PerfHelper ph;
           ph.begin();
           if(!sOcrAPI->inferBatchWithLock(in.ids, in.base64Imgs, &out.results)){
                out.msg = "infer failed";
           }
           ph.print("infer_cost");
       }
       json _json;
       ocr::json_impl::to_json(_json, out);
       return _json.dump();
   };
   printf("startHttpService >> now... port = %d, tc = %d\n", port, tc);
   startHttpService(port, tc, {reqItem});
}

