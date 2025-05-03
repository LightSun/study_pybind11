#pragma once

//#include "fastdeploy/vision.h"
#include "utils.h"

//https://github.com/PaddlePaddle/FastDeploy/blob/release/1.0.2/examples/vision/detection/paddledetection/cpp/infer_picodet.cc
namespace h7 {
/*

class FD_Picodet : public FD_InferBase
{
public:
    ~FD_Picodet(){
       if(m_ptr){
           delete m_ptr;
           m_ptr = nullptr;
       }
    }

    bool loadConfig(CString cache_dir, MedDataEncDec* mded){
         return true;
    }

    bool loadConfig(const std::string& model_dir){
        MED_ASSERT(m_ptr == nullptr);
        auto model_file = model_dir + SEP + "model.pdmodel";
        auto params_file = model_dir + SEP + "model.pdiparams";
        auto config_file = model_dir + SEP + "infer_cfg.yml";
        auto option = fastdeploy::RuntimeOption();
        //applyOptions(option, &m_config);
        m_ptr = new fastdeploy::vision::detection::PicoDet(model_file, params_file,
                                                       config_file, option);
        return m_ptr->Initialized();
    }

    bool infer(cv::Mat& mat, fastdeploy::vision::DetectionResult* res){
        MED_ASSERT(m_ptr != nullptr);
        return m_ptr->Predict(mat, res);
    }

    bool inferBatch(const std::vector<cv::Mat>& imgs,
                       std::vector<fastdeploy::vision::DetectionResult>* results){
        MED_ASSERT(m_ptr != nullptr);
        return m_ptr->BatchPredict(imgs, results);
    }

private:
    fastdeploy::vision::detection::PicoDet* m_ptr {nullptr};
};
*/
}
