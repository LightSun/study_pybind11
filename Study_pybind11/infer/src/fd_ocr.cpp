#include "fastdeploy/vision.h"
#include "fd_ocr.h"
#include "EDMHelper.h"
#include "utils.h"
#include "ocr_common.h"
#include "core/src/base64.h"

namespace h7 {

class FD_OCR: public FD_InferBase
{
public:

    using UpFDModule = std::unique_ptr<fastdeploy::FastDeployModel>;
    using FDModule = fastdeploy::FastDeployModel;
    using OcrPipiline = fastdeploy::pipeline::PPOCRv4;

    bool loadConfigs();
    bool loadConfigs(EdmHelper* decoder);

    bool buildOCR();

    bool infer(cv::Mat& mat, fastdeploy::vision::OCRResult* res){
        MED_ASSERT(m_ptr != nullptr);
        return m_ptr->Predict(mat, res);
    }

    bool inferBatch(const std::vector<cv::Mat>& imgs,
                       std::vector<fastdeploy::vision::OCRResult>* results){
        MED_ASSERT(m_ptr != nullptr);
        return m_ptr->BatchPredict(imgs, results);
    }
    bool inferWithLock(cv::Mat& mat, fastdeploy::vision::OCRResult* res){
        MED_ASSERT(m_ptr != nullptr);
        std::unique_lock<std::mutex> lck(m_mtx);
        return m_ptr->Predict(mat, res);
    }

    bool inferBatchWithLock(const std::vector<cv::Mat>& imgs,
                       std::vector<fastdeploy::vision::OCRResult>* results){
        MED_ASSERT(m_ptr != nullptr);
        std::unique_lock<std::mutex> lck(m_mtx);
        return m_ptr->BatchPredict(imgs, results);
    }
private:
    bool loadConfig(int type, EdmHelper* decoder,
                    int batch_size = 6);
    bool loadConfigCh(int type, EdmHelper* decoder,
                    int batch_size);

    bool loadConfigEn(int type, EdmHelper* decoder,
                    int batch_size);
    bool loadConfig(int type, CString model_dir,int batch_size = 6,
                    CString rec_label_file="");

private:
    template<typename T>
    T* getFDModulePtr(int type){
        return (T*)m_modules[type].get();
    }

private:
    std::map<int,UpFDModule> m_modules;
    std::unique_ptr<OcrPipiline> m_ptr;
    std::mutex m_mtx;
};
}

namespace h7 {

static inline void SetTrtInputShape(int run_mode,fastdeploy::RuntimeOption& option,
        const std::string& input_name, const std::vector<int32_t>& min_shape,
        const std::vector<int32_t>& opt_shape = std::vector<int32_t>(),
        const std::vector<int32_t>& max_shape = std::vector<int32_t>()){
    switch (run_mode) {
    case kTRT_F32:
    case kTRT_INT8:
    case kTRT_F16:{
        option.trt_option.SetShape(input_name, min_shape, opt_shape, max_shape);
    }break;
    }
}

static inline void applyOptions(fastdeploy::RuntimeOption& option, OcrConfig* in){
    switch (in->run_mode) {
    case kPADDLE:{
        if(in->device == "GPU"){
            option.UseGpu(in->gpu_id);
        }else{
            option.UseCpu();
        }
        option.UsePaddleBackend();
    }break;

    case kTRT_F32:{
        option.UseGpu(in->gpu_id);
        option.UseTrtBackend();
    }break;

    case kTRT_INT8:{
        println("'kTRT_INT8' is not support for FP. use default fp16 now.");
    }
    case kTRT_F16:{
        option.UseGpu(in->gpu_id);
        option.UseTrtBackend();
        option.EnableTrtFP16();
    }break;

    case kOpenvino:{
        option.UseCpu();
        option.UseOpenVINOBackend();
    }break;

    default:
        MED_ASSERT_X(false, "unsupport runMode = " << in->run_mode);
        break;
    }
}


bool FD_OCR::loadConfigs(){
    if(!m_config.model_desc_dir.empty()){
        EdmHelper edmH;
        edmH.loadDir(m_config.model_desc_dir);
        return loadConfigs(&edmH);
    }else{
        return loadConfigs(nullptr);
    }
}

bool FD_OCR::loadConfigs(EdmHelper* decoder){
    if(!m_config.model_desc_dir.empty()){
        FDASSERT(!m_config.cache_dir.empty(), "must set cache_dir");
        if(!loadConfig(kOCR_Type_DET, decoder, 0)){
            return false;
        }
        if(!loadConfig(kOCR_Type_CLS, decoder, m_config.max_batch_cls)){
            return false;
        }
        if(!loadConfig(kOCR_Type_REC, decoder, m_config.max_batch_rec)){
            return false;
        }
    }else{
        FDASSERT(!m_config.model_dir_det.empty(), "must set 'model_dir_det'");
        FDASSERT(!m_config.model_dir_cls.empty(), "must set 'model_dir_cls'");
        FDASSERT(!m_config.model_dir_rec.empty(), "must set 'model_dir_rec'");
        FDASSERT(!m_config.rec_label_file.empty(), "must set 'rec_label_file'");
        if(!loadConfig(kOCR_Type_DET, m_config.model_dir_det, 0)){
            return false;
        }
        if(!loadConfig(kOCR_Type_CLS, m_config.model_dir_cls, m_config.max_batch_cls)){
            return false;
        }
        if(!loadConfig(kOCR_Type_DET, m_config.model_dir_rec, m_config.max_batch_rec)){
            return false;
        }
    }
    return true;
}
bool FD_OCR::loadConfig(int type, EdmHelper* decoder,
                int batch_size){
    switch (m_config.ocrVersion) {
    case kOCR_V4_CH:{
        return loadConfigCh(type, decoder, batch_size);
    }break;

    case kOCR_V4_EN:{
        return loadConfigEn(type, decoder, batch_size);
    }break;

    default:
        fprintf(stderr, "loadConfig >> unsupport ocrVersion = %d !!!\n", m_config.ocrVersion);
    }
    return false;
}

bool FD_OCR::loadConfigCh(int type, EdmHelper* decoder,
                int batch_size){
    auto& cache_dir = m_config.cache_dir;
    auto rec_label_file = cache_dir + "/c1.d0";
    {
        String ocr_keys = decoder->getItemData("/ocrv4_rec/ppocr_keys_v1.txt");
        FDASSERT(!ocr_keys.empty(), "load en_dict failed!");
        Utils::writeFile(rec_label_file, ocr_keys);
    }
    //
    auto option = fastdeploy::RuntimeOption();
    applyOptions(option, &m_config);
    //
    using OcrDet = fastdeploy::vision::ocr::DBDetector;
    using OcrRec = fastdeploy::vision::ocr::Recognizer;
    using OcrCls = fastdeploy::vision::ocr::Classifier;
    //
    UpFDModule ins;
    switch (type) {
    case kOCR_Type_DET:{

        SetTrtInputShape(m_config.run_mode, option, "x", {1, 3, 64,64}, {1, 3, 640, 640},
                {1, 3, 960, 960}
        );
        auto model = decoder->getItemData("/ocrv4_det/inference.pdmodel");
        auto pdi = decoder->getItemData("/ocrv4_det/inference.pdiparams");
        FDASSERT(!model.empty(), "load det_db module failed!");
        FDASSERT(!pdi.empty(), "load det_db pdi failed!");
//        option.SetModelBuffer(model, pdi, fastdeploy::ModelFormat::PADDLE);
//        String model_file;
//        String params_file;
        auto model_file = cache_dir + "/c1.d2";
        auto params_file = cache_dir + "/c1.d3";
        Utils::writeFile(model_file, model);
        Utils::writeFile(params_file, pdi);
        ins = std::unique_ptr<OcrDet>(new OcrDet(model_file,
                       params_file, option));
       } break;

    case kOCR_Type_CLS:{
        SetTrtInputShape(m_config.run_mode, option, "x", {1, 3, 48, 10},
            {batch_size, 3, 48, 320},
            {batch_size, 3, 48, 1024}
        );

        auto model = decoder->getItemData("/ocrv4_cls/inference.pdmodel");
        auto pdi = decoder->getItemData("/ocrv4_cls/inference.pdiparams");
        FDASSERT(!model.empty(), "load cls_db module failed!");
        FDASSERT(!pdi.empty(), "load cls_db pdi failed!");
//        option.SetModelBuffer(model, pdi, fastdeploy::ModelFormat::PADDLE);
//        String model_file;
//        String params_file;
        auto model_file = cache_dir + "/c1.d5";
        auto params_file = cache_dir + "/c1.d6";
        Utils::writeFile(model_file, model);
        Utils::writeFile(params_file, pdi);
        ins = std::unique_ptr<OcrCls>(new OcrCls(model_file,
                       params_file, option));
    }break;

    case kOCR_Type_REC:{
        SetTrtInputShape(m_config.run_mode, option, "x", {1, 3, 48, 10},
            {batch_size, 3, 48, 320},
            {batch_size, 3, 48, 2304}
        );
        auto model = decoder->getItemData("/ocrv4_rec/inference.pdmodel");
        auto pdi = decoder->getItemData("/ocrv4_rec/inference.pdiparams");
        FDASSERT(!model.empty(), "load rec module failed!");
        FDASSERT(!pdi.empty(), "load rec pdi failed!");
//        option.SetModelBuffer(model, pdi, fastdeploy::ModelFormat::PADDLE);
//        String model_file;
//        String params_file;
        auto model_file = cache_dir + "/c1.d8";
        auto params_file = cache_dir + "/c1.d9";
        Utils::writeFile(model_file, model);
        Utils::writeFile(params_file, pdi);
        ins = std::unique_ptr<OcrRec>(new OcrRec(model_file,
                       params_file, rec_label_file, option));
    }break;

    default:
        return false;
    }
    MED_ASSERT(ins->Initialized());
    m_modules[type] = std::move(ins);
    return true;
}

bool FD_OCR::loadConfigEn(int type, EdmHelper* decoder,
                int batch_size){
    auto& cache_dir = m_config.cache_dir;
    auto rec_label_file = cache_dir + "/c11.d0";
    {
        String ocr_keys = decoder->getItemData("/en/ocrv4_rec/en_dict.txt");
        FDASSERT(!ocr_keys.empty(), "load en_dict failed!");
        Utils::writeFile(rec_label_file, ocr_keys);
    }
    //
    auto option = fastdeploy::RuntimeOption();
    applyOptions(option, &m_config);
    //
    using OcrDet = fastdeploy::vision::ocr::DBDetector;
    using OcrRec = fastdeploy::vision::ocr::Recognizer;
    using OcrCls = fastdeploy::vision::ocr::Classifier;
    //
    UpFDModule ins;
    switch (type) {
    case kOCR_Type_DET:{

        SetTrtInputShape(m_config.run_mode, option, "x", {1, 3, 64,64}, {1, 3, 640, 640},
                {1, 3, 960, 960}
        );
        auto model = decoder->getItemData("/en/ocrv3_det/inference.pdmodel");
        auto pdi = decoder->getItemData("/en/ocrv3_det/inference.pdiparams");
        FDASSERT(!model.empty(), "load det_db module failed!");
        FDASSERT(!pdi.empty(), "load det_db pdi failed!");
        auto model_file = cache_dir + "/c11.d2";
        auto params_file = cache_dir + "/c11.d3";
        Utils::writeFile(model_file, model);
        Utils::writeFile(params_file, pdi);
        ins = std::unique_ptr<OcrDet>(new OcrDet(model_file,
                       params_file, option));
       } break;

    case kOCR_Type_CLS:{
        SetTrtInputShape(m_config.run_mode, option, "x", {1, 3, 48, 10},
            {batch_size, 3, 48, 320},
            {batch_size, 3, 48, 1024}
        );

        auto model = decoder->getItemData("/ocrv4_cls/inference.pdmodel");
        auto pdi = decoder->getItemData("/ocrv4_cls/inference.pdiparams");
        FDASSERT(!model.empty(), "load cls_db module failed!");
        FDASSERT(!pdi.empty(), "load cls_db pdi failed!");
        auto model_file = cache_dir + "/c11.d5";
        auto params_file = cache_dir + "/c11.d6";
        Utils::writeFile(model_file, model);
        Utils::writeFile(params_file, pdi);
        ins = std::unique_ptr<OcrCls>(new OcrCls(model_file,
                       params_file, option));
    }break;

    case kOCR_Type_REC:{
        SetTrtInputShape(m_config.run_mode, option, "x", {1, 3, 48, 10},
            {batch_size, 3, 48, 320},
            {batch_size, 3, 48, 2304}
        );
        auto model = decoder->getItemData("/en/ocrv4_rec/inference.pdmodel");
        auto pdi = decoder->getItemData("/en/ocrv4_rec/inference.pdiparams");
        FDASSERT(!model.empty(), "load rec module failed!");
        FDASSERT(!pdi.empty(), "load rec pdi failed!");
        auto model_file = cache_dir + "/c11.d8";
        auto params_file = cache_dir + "/c11.d9";
        Utils::writeFile(model_file, model);
        Utils::writeFile(params_file, pdi);
        ins = std::unique_ptr<OcrRec>(new OcrRec(model_file,
                       params_file, rec_label_file, option));
    }break;

    default:
        return false;
    }
    MED_ASSERT(ins->Initialized());
    m_modules[type] = std::move(ins);
    return true;
}

bool FD_OCR::loadConfig(int type, CString model_dir,int batch_size,
                CString rec_label_file){
    auto model_file = model_dir + SEP + "inference.pdmodel";
    auto params_file = model_dir + SEP + "inference.pdiparams";
    auto option = fastdeploy::RuntimeOption();
    applyOptions(option, &m_config);
    //option.SetTrtCacheFile()

    UpFDModule ins;
    switch (type) {
    case kOCR_Type_DET:
        SetTrtInputShape(m_config.run_mode, option, "x", {1, 3, 64,64}, {1, 3, 640, 640},
                {1, 3, 960, 960});
        ins = std::unique_ptr<FDModule>(new fastdeploy::vision::ocr::DBDetector(model_file,
                       params_file, option));
        break;

    case kOCR_Type_CLS:
        SetTrtInputShape(m_config.run_mode, option, "x", {1, 3, 48, 10},
            {batch_size, 3, 48, 320},
            {batch_size, 3, 48, 1024}
        );
        ins = std::unique_ptr<FDModule>(new fastdeploy::vision::ocr::Classifier(
                                         model_file, params_file, option));
        break;

    case kOCR_Type_REC:
        SetTrtInputShape(m_config.run_mode, option, "x", {1, 3, 48, 10},
            {batch_size, 3, 48, 320},
            {batch_size, 3, 48, 2304}
        );
        ins = std::unique_ptr<FDModule>(new fastdeploy::vision::ocr::Recognizer(model_file,
                       params_file, rec_label_file, option));
        break;

    default:
        return false;
    }
    MED_ASSERT(ins->Initialized());
    m_modules[type] = std::move(ins);
    return true;
}

bool FD_OCR::buildOCR(){
    MED_ASSERT_X(m_ptr == nullptr, "ocr module already build.");
    MED_ASSERT(m_modules[kOCR_Type_DET] != nullptr);
    MED_ASSERT(m_modules[kOCR_Type_REC] != nullptr);
    using OcrDet = fastdeploy::vision::ocr::DBDetector;
    using OcrRec = fastdeploy::vision::ocr::Recognizer;
    using OcrCls = fastdeploy::vision::ocr::Classifier;
    if(m_modules[kOCR_Type_CLS] == nullptr){
        m_ptr = std::unique_ptr<OcrPipiline>(new OcrPipiline(
                    getFDModulePtr<OcrDet>(kOCR_Type_DET),
                    getFDModulePtr<OcrRec>(kOCR_Type_REC)));
    }else{
        m_ptr = std::unique_ptr<OcrPipiline>(new OcrPipiline(
                    getFDModulePtr<OcrDet>(kOCR_Type_DET),
                    getFDModulePtr<OcrCls>(kOCR_Type_CLS),
                    getFDModulePtr<OcrRec>(kOCR_Type_REC)));
    }
    int cls_batch = m_config.max_batch_cls;
    int rec_batch = m_config.max_batch_rec;
    if(cls_batch > 0){
        m_ptr->SetClsBatchSize(cls_batch);
    }
    if(rec_batch > 0){
        m_ptr->SetRecBatchSize(rec_batch);
    }
    return m_ptr->Initialized();
}

//---------------------------------

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

bool OcrApi::infer(cv::Mat& mat, OcrResult* _res){
    auto& res = *_res;
    fastdeploy::vision::OCRResult kret;
    if(!infer(mat, &kret)){
        return false;
    }
    res.texts = std::move(kret.text);
    res.boxes = std::move(kret.boxes);
    res.rec_scores = std::move(kret.rec_scores);
    res.cls_scores = std::move(kret.cls_scores);
    res.cls_labels = std::move(kret.cls_labels);
    return true;
}

bool OcrApi::inferWithLock(cv::Mat& mat, OcrResult* _res){
    auto& res = *_res;
    fastdeploy::vision::OCRResult kret;
    if(!inferWithLock(mat, &kret)){
        return false;
    }
    res.texts = std::move(kret.text);
    res.boxes = std::move(kret.boxes);
    res.rec_scores = std::move(kret.rec_scores);
    res.cls_scores = std::move(kret.cls_scores);
    res.cls_labels = std::move(kret.cls_labels);
    return true;
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

static inline bool base64ToMat(CString id,CString base64, cv::Mat& mat){
    auto _imgBuf = base64_decode(base64);
    std::vector<char> imgBuf(_imgBuf.begin(), _imgBuf.end());
    try {
        mat = cv::imdecode(imgBuf, cv::IMREAD_COLOR);
        if(mat.empty()){
            fprintf(stderr, "ImageInferImpl::infer >> wrong data: img id = %s !!!\n", id.data());
            return false;
        }
        cv::imwrite("test.png", mat);
    } catch (cv::Exception& e) {
        fprintf(stderr, "cv::imdecode >> error: id = %s, msg = %s\n",
                id.data(), e.what());
        return false;
    }
    return true;
}
bool OcrApi::inferWithLock(CString id,CString base64, fastdeploy::vision::OCRResult* res){
    cv::Mat mat;
    if(!base64ToMat(id, base64, mat)){
        return false;
    }
    return inferWithLock(mat, res);
}
bool OcrApi::inferBatchWithLock(const std::vector<String>& ids,
                                const std::vector<String>& base64s,
                        std::vector<fastdeploy::vision::OCRResult>* results){
    MED_ASSERT(ids.size() == base64s.size());
    std::vector<cv::Mat> mats;
    for(int i = 0 ; i < (int)ids.size() ; ++i){
        cv::Mat mat;
        if(!base64ToMat(ids[i], base64s[i], mat)){
            return false;
        }
        mats.push_back(mat);
    }
    return inferBatchWithLock(mats, results);
}

}
