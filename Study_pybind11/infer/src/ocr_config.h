#pragma once

#include <string>
#include <memory>

#define USE_OPENVONO 1
//#define USE_MED_RSA 1
//#define USE_TABLE_API 1

namespace h7 {

using String = std::string;

enum RunModeType{
    kPADDLE = 1,
    kTRT_F32,
    kTRT_F16,
    kTRT_INT8,
    kOpenvino
};

enum OcrVersion{
    kOCR_V4_CH,
    kOCR_V4_EN,
};

struct ExtOcrConfig{
    int numThread {2};
    int loopCount {1};
    int padding {50};
    int maxSideLen {1024};
    float boxScoreThresh {0.5f};
    float boxThresh {0.3f};
    float unClipRatio {1.0f}; //raw 1.5f
    int flagDoAngle {1};
    bool doAngle {true};
    bool mostAngle {true};
};

struct OcrConfig{
    String cache_dir;
    String model_desc_dir;

    String model_dir_cls;
    String model_dir_det;
    String model_dir_rec;
    String rec_label_file;

    String device {"GPU"}; //GPU or CPU

    float threshold {0.5f};
    int ocrVersion {kOCR_V4_CH};
    int gpu_id {0};
    int run_mode {kPADDLE};
    int max_batch_cls {6};
    int max_batch_rec {8};
    bool debug {false};
    String logFile;

    std::shared_ptr<ExtOcrConfig> extConfig;

    String getRunMode(){
        switch (run_mode) {
        default:
        case kPADDLE:
            return "PADDLE";

        case kTRT_F32:
            return "TRT_F32";

        case kTRT_F16:
            return "TRT_F16";

        case kTRT_INT8:
            return "TRT_INT8";
        case kOpenvino:
            return "OPENVINO";
        }
    }
    void setRunMode(const String& mode){
        if(mode == "PADDLE"){
            run_mode = kPADDLE;
        }else if(mode == "TRT_F32"){
            run_mode = kTRT_F32;
        }else if(mode == "TRT_F16"){
            run_mode = kTRT_F16;
        }else if(mode == "TRT_INT8"){
            run_mode = kTRT_INT8;
        }else if(mode == "OPENVINO"){
            run_mode = kOpenvino;
        }else{
            fprintf(stderr, "wrong run mode for OcrInfer.");
            abort();
        }
    }
};

}

