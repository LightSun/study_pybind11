
#include "core/src/Prop.h"
#include "IHttp.h"

using namespace med_qa;

int main(int argc, const char* argv[]){

    if(argc == 1){
        String conf = "/media/heaven7/Elements_SE/study/work/OCR/local"
                              "/test.config";
        std::vector<String> args = {
            "HttpOcrInfer",
            conf,
        };
        const char* argvs[args.size()];
        for(int i = 0 ; i < (int)args.size() ; i ++){
            argvs[i] = args[i].c_str();
        }
        return main(args.size(), argvs);
    }
    setbuf(stdout, NULL);

    Prop prop;
    prop.load(argv[1]);
    //
    med_http::startService(&prop);
    return 0;
}
