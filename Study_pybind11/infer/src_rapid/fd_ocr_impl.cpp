#include "../src/fd_ocr.h"
#include "../src/EDMHelper.h"
#include "../src/utils.h"
#include "OcrLite.h"
#include <memory>

using namespace h7;

namespace h7 {

class FD_OCR: public FD_InferBase
{
public:
    bool loadConfigs(){
        if(m_config.model_desc_dir.empty()){
            return false;
        }
        EdmHelper edmH;
        edmH.loadDir(m_config.model_desc_dir);
        return loadConfigs(&edmH);
    }
    bool loadConfigs(EdmHelper* decoder){
        if(m_config.model_desc_dir.empty()){
            return false;
        }
        m_ocrLite = std::unique_ptr<OcrLite>(new OcrLite());
        if(m_config.ocrVersion == kOCR_V4_CH){
            return loadConfigsCh(decoder);
        }
        return loadConfigsEn(decoder);
    }
    bool buildOCR(){
        return true;
    }
    bool infer(cv::Mat& , fastdeploy::vision::OCRResult* ){
        FDASSERT(false, "fastdeploy::vision::OCRResult not support.");
        return false;
    }

    bool inferBatch(const std::vector<cv::Mat>& ,
                    std::vector<fastdeploy::vision::OCRResult>* ){
        FDASSERT(false, "fastdeploy::vision::OCRResult not support.");
        return false;
    }

    bool inferWithLock(cv::Mat& , fastdeploy::vision::OCRResult* ){
        FDASSERT(false, "fastdeploy::vision::OCRResult not support.");
        return false;
    }

    bool inferBatchWithLock(const std::vector<cv::Mat>& ,
                            std::vector<fastdeploy::vision::OCRResult>* ){
        FDASSERT(false, "fastdeploy::vision::OCRResult not support.");
        return false;
    }

    bool inferWithLock(CString ,CString , fastdeploy::vision::OCRResult* ){
        FDASSERT(false, "fastdeploy::vision::OCRResult not support.");
        return false;
    }

    bool inferBatchWithLock(const std::vector<String>& ,
                            const std::vector<String>& ,
                            std::vector<fastdeploy::vision::OCRResult>* ){
        FDASSERT(false, "fastdeploy::vision::OCRResult not support.");
        return false;
    }
    //---------------
    bool infer(cv::Mat& mat, OcrResult* res){
        MED_ASSERT(m_config.extConfig);
        *res = OcrResult();
        auto& etc = *m_config.extConfig;
        auto result = m_ocrLite->detect(mat,
                                      etc.padding, etc.maxSideLen,
                                      etc.boxScoreThresh, etc.boxThresh,
                                      etc.unClipRatio,
                                      etc.doAngle, etc.mostAngle);
        //res->boxes
        for(auto& ret: result.textBlocks){
            MED_ASSERT(ret.boxPoint.size() == 4);
            std::array<int,8> boxes;
            boxes[0] = ret.boxPoint[0].x;
            boxes[1] = ret.boxPoint[0].y;
            boxes[2] = ret.boxPoint[1].x;
            boxes[3] = ret.boxPoint[1].y;
            boxes[4] = ret.boxPoint[2].x;
            boxes[5] = ret.boxPoint[2].y;
            boxes[6] = ret.boxPoint[3].x;
            boxes[7] = ret.boxPoint[3].y;
            float textScore = 0;
            for(auto& f: ret.charScores){
                textScore += f;
            }
            if(!ret.charScores.empty()){
                textScore /= ret.charScores.size();
            }
            res->boxes.push_back(std::move(boxes));
            res->texts.push_back(ret.text);
            res->rec_scores.push_back(textScore);
            if(m_config.ocrVersion == kOCR_V4_CH){
                res->cls_labels.push_back(ret.angleIndex);
                res->cls_scores.push_back(ret.angleScore);
            }
        }
        return true;
    }

    bool inferWithLock(cv::Mat& mat, OcrResult* res){
        std::unique_lock<std::mutex> mtx(m_mtx);
        return infer(mat, res);
    }

private:
    bool loadConfigsCh(EdmHelper* decoder){
        if(m_config.model_desc_dir.empty()){
            return false;
        }
        auto& etc = *m_config.extConfig;
        auto det_file = loadToFile(decoder, "/ch_PP-OCRv4_det_infer.onnx",
                                   "ot1.d0");
        auto cls_file = loadToFile(decoder, "/ch_ppocr_mobile_v2.0_cls_infer.onnx",
                                   "ot2.d0");
        auto rec_file = loadToFile(decoder, "/ch_PP-OCRv4_rec_infer.onnx",
                                   "ot3.d0");
        auto keys_file = loadToFile(decoder, "/ppocr_keys_v1.txt",
                                   "ot4.d0");
        m_ocrLite->setNumThread(etc.numThread);
        m_ocrLite->initLogger(false, false, false);
        m_ocrLite->setGpuIndex(m_config.gpu_id);
        return m_ocrLite->initModels(det_file, cls_file, rec_file, keys_file);
    }
    bool loadConfigsEn(EdmHelper* decoder){
        if(m_config.model_desc_dir.empty()){
            return false;
        }
        auto& etc = *m_config.extConfig;
        etc.doAngle = false; // for en. no need
        auto det_file = loadToFile(decoder, "/en_PP-OCRv3_det_infer.onnx",
                                   "ot1.d1");
        auto rec_file = loadToFile(decoder, "/en_PP-OCRv4_rec_infer.onnx",
                                   "ot2.d1");
        auto keys_file = loadToFile(decoder, "/en_dict.txt",
                                   "ot3.d1");
        m_ocrLite->setNumThread(etc.numThread);
        m_ocrLite->initLogger(false, false, false);
        m_ocrLite->setGpuIndex(m_config.gpu_id);
        return m_ocrLite->initModels(det_file, "", rec_file, keys_file);
    }
    String loadToFile(EdmHelper* decoder, CString key, CString saveName){
        auto& cache_dir = m_config.cache_dir;
        auto target_file = cache_dir + "/" + saveName;
        {
            String cs = decoder->getItemData(key);
            FDASSERT(!cs.empty(), "load '%s' failed!", key.data());
            Utils::writeFile(target_file, cs);
        }
        return target_file;
    }
private:
    std::unique_ptr<OcrLite> m_ocrLite;
    std::mutex m_mtx;
};
//---------------------
OcrApi::OcrApi(){
    m_ptr = new FD_OCR();
}
OcrApi::~OcrApi(){
    if(m_ptr){
        delete m_ptr;
        m_ptr = nullptr;
    }
}
void OcrApi::setOcrConfig(const OcrConfig& c){
    m_ptr->setOcrConfig(c);
}
bool OcrApi::loadConfigs(){
    return m_ptr->loadConfigs();
}
bool OcrApi::loadConfigs(EdmHelper* decoder){
    return m_ptr->loadConfigs(decoder);
}
bool OcrApi::buildOCR(){
    return m_ptr->buildOCR();
}

bool OcrApi::infer(cv::Mat& mat, fastdeploy::vision::OCRResult* res){
    return m_ptr->infer(mat, res);
}

bool OcrApi::inferBatch(const std::vector<cv::Mat>& imgs,
                        std::vector<fastdeploy::vision::OCRResult>* results){
    return m_ptr->inferBatch(imgs, results);
}

bool OcrApi::inferWithLock(cv::Mat& mat, fastdeploy::vision::OCRResult* res){
    return m_ptr->inferWithLock(mat, res);
}

bool OcrApi::inferBatchWithLock(const std::vector<cv::Mat>& imgs,
                                std::vector<fastdeploy::vision::OCRResult>* results){
    return m_ptr->inferBatchWithLock(imgs, results);
}

bool OcrApi::inferWithLock(CString id,CString base64,
                           fastdeploy::vision::OCRResult* res){
    return m_ptr->inferWithLock(id, base64, res);
}

bool OcrApi::inferBatchWithLock(const std::vector<String>& ids,
                        const std::vector<String>& base64s,
                                std::vector<fastdeploy::vision::OCRResult>* results){
    return m_ptr->inferBatchWithLock(ids, base64s, results);
}
//---------------
bool OcrApi::infer(cv::Mat& mat, OcrResult* res){
    return m_ptr->infer(mat, res);
}
bool OcrApi::inferWithLock(cv::Mat& mat, OcrResult* res){
    return m_ptr->inferWithLock(mat, res);
}
}
