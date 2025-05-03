
#include "opencv2/opencv.hpp"
#include "../src/fd_ocr.h"
#include "../src/ocr_common.h"

using namespace h7;

int main(int argc, const char* argv[]){
    OcrConfig cfg;
    cfg.extConfig = std::make_shared<ExtOcrConfig>();
    cfg.cache_dir = "/media/heaven7/Elements_SE/study/work/"
                    "MedQA/V2/OneKeyInstall_dev/main/cache";
    cfg.model_desc_dir = "/media/heaven7/Elements_SE/study/"
                         "work/MedQA/meinian/RapidOcr_desc";
    cfg.gpu_id = 0;
    cfg.ocrVersion = kOCR_V4_EN;
    //
    String img = "/media/heaven7/Elements_SE/study/work/MedQA/meinian/RapidOcr/1.jpg";
    auto mat = cv::imread(img);
    //
    OcrApi api;
    api.setOcrConfig(cfg);
    MED_ASSERT(api.loadConfigs());
    api.buildOCR();
    OcrResult ret;
    MED_ASSERT(api.inferWithLock(mat, &ret));

    return 0;
}
