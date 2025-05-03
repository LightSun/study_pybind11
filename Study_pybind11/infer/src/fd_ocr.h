#pragma once

#include "infer/src/ocr_context.h"

namespace fastdeploy {
namespace vision {
struct OCRResult;
}
}

namespace h7 {

class EdmHelper;
class FD_OCR;

enum{
    kOCR_Type_DET,
    kOCR_Type_CLS,
    kOCR_Type_REC,
};

struct OcrResult{
  std::vector<std::array<int, 8>> boxes;

  std::vector<std::string> texts;
  std::vector<float> rec_scores;

  std::vector<float> cls_scores;
  std::vector<int32_t> cls_labels;

  void addTo(int curI, OcrResult& out){
      out.boxes.push_back(boxes[curI]);
      out.texts.push_back(texts[curI]);
      out.rec_scores.push_back(rec_scores[curI]);
      if(!cls_scores.empty()){
          out.cls_scores.push_back(cls_scores[curI]);
      }
      if(!cls_labels.empty()){
          out.cls_labels.push_back(cls_labels[curI]);
      }
  }
};

class OcrApi: public INoCopy
{
public:
    OcrApi();
    ~OcrApi();

    void setOcrConfig(const OcrConfig&);

    bool loadConfigs();
    bool loadConfigs(EdmHelper* decoder);

    bool buildOCR();

    bool infer(cv::Mat& mat, fastdeploy::vision::OCRResult* res);

    bool inferBatch(const std::vector<cv::Mat>& imgs,
                       std::vector<fastdeploy::vision::OCRResult>* results);

    bool inferWithLock(cv::Mat& mat, fastdeploy::vision::OCRResult* res);

    bool inferBatchWithLock(const std::vector<cv::Mat>& imgs,
                       std::vector<fastdeploy::vision::OCRResult>* results);

    bool inferWithLock(CString id,CString base64, fastdeploy::vision::OCRResult* res);

    bool inferBatchWithLock(const std::vector<String>& ids,
                            const std::vector<String>& base64s,
                       std::vector<fastdeploy::vision::OCRResult>* results);
    //---------------
    bool infer(cv::Mat& mat, OcrResult* res);

    bool inferWithLock(cv::Mat& mat, OcrResult* res);


private:
    FD_OCR* m_ptr;
};

}

