#include "IHttp.h"
#include "hv/HttpServer.h"
#include "infer/src/fd_ocr.h"

using namespace med_http;

void med_http::startHttpService(int port, int tc, const std::vector<HttpReqItem>& items){
    HttpService router;
    for(auto& item : items){
        MED_ASSERT(!item.reqPath.empty());
        MED_ASSERT(item.func_processor);
        auto callback = [item](const HttpContextPtr& ctx){
                         auto body = ctx->body();
                         auto res = item.func_processor(body);
                         int ret = ctx->send(res, ctx->type());
                         return ret;
        };
        if(item.isGet){
            router.GET(item.reqPath.data(), callback);
        }else{
            router.POST(item.reqPath.data(), callback);
        }
    }
    //
    hv::HttpServer server(&router);
    server.setPort(port);
    server.setThreadNum(tc);
    server.run();
}
