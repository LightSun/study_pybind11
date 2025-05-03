// Copyright (c) 2022 PaddlePaddle Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "fastdeploy/vision.h"
#include "pybind/src/binds.h"
#include "infer/src/ocr_context.h"
#include "infer/src/fd_ocr.h"
#include "infer/src/EDMHelper.h"
#include "pyutils.h"

namespace h7 {

void BindEDM(pybind11::module& m){
    pybind11::class_<EdmHelper>(m, "EdmHelper")
            .def(pybind11::init<>())
            .def("loadDir",[](EdmHelper& self, CString dir){
                self.loadDir(dir);
            })
            .def("loadDesc",[](EdmHelper& self, CString desc){
                self.loadDesc(desc);
            })
            .def("getItem",[](EdmHelper& self, CString key){
                return self.getItem(key);
            })
            ;
}

void BindOcrInfer(pybind11::module& m){
    using OCRResult = fastdeploy::vision::OCRResult;
    pybind11::enum_<RunModeType>(m, "RunMode", pybind11::arithmetic(),
                                 "RunMode for inference.")
            .value("PADDLE", RunModeType::kPADDLE)
            .value("TRT_F32", RunModeType::kTRT_F32)
            .value("TRT_F16", RunModeType::kTRT_F16)
            .value("TRT_INT8", RunModeType::kTRT_INT8)
            .value("OPENVINO", RunModeType::kOpenvino);
    //
    pybind11::class_<OcrConfig>(m, "OcrConfig")
            .def(pybind11::init<>())
            .def_readwrite("cache_dir", &OcrConfig::cache_dir)
            .def_readwrite("model_desc_dir", &OcrConfig::model_desc_dir)
            .def_readwrite("model_dir_cls", &OcrConfig::model_dir_cls)
            .def_readwrite("model_dir_det", &OcrConfig::model_dir_det)
            .def_readwrite("model_dir_rec", &OcrConfig::model_dir_rec)
            .def_readwrite("rec_label_file", &OcrConfig::rec_label_file)
            .def_readwrite("device", &OcrConfig::device)
            .def_readwrite("gpu_id", &OcrConfig::gpu_id)
            //.def_readwrite("run_mode", &OcrConfig::run_mode)
            .def_readwrite("max_batch_cls", &OcrConfig::max_batch_cls)
            .def_readwrite("max_batch_rec", &OcrConfig::max_batch_rec)
            .def_readwrite("debug", &OcrConfig::debug)
            .def_readwrite("logFile", &OcrConfig::logFile)
            .def_property("run_mode", &OcrConfig::getRunMode, &OcrConfig::setRunMode)
            ;

    pybind11::class_<OCRResult>(m, "OCRResult")
          .def(pybind11::init())
          .def_readwrite("boxes", &OCRResult::boxes)
          .def_readwrite("texts", &OCRResult::text)
          .def_readwrite("rec_scores", &OCRResult::rec_scores)
          .def_readwrite("cls_scores", &OCRResult::cls_scores)
          .def_readwrite("cls_labels", &OCRResult::cls_labels)
          .def("__repr__", &OCRResult::Str)
          .def("__str__", &OCRResult::Str)
            ;

    pybind11::class_<OcrApi>(m, "OcrApi")
            .def(pybind11::init<>())
            .def("setConfig", [](OcrApi& self, const OcrConfig& c){
                self.setOcrConfig(c);
            })
            .def("loadConfigs", [](OcrApi& self){
                return self.loadConfigs();
            })
            .def("loadConfigs", [](OcrApi& self, EdmHelper& h){
                return self.loadConfigs(&h);
            })
            .def("buildOCR", [](OcrApi& self){
                return self.buildOCR();
            })
            .def("infer", [](OcrApi& self, pybind11::array& np){
                FDASSERT(np.ndim() == 3, "must: np.ndim() == 3");
//                auto buf = np.request();
//                cv::Mat mat(static_cast<int>(buf.shape[0]), static_cast<int>(buf.shape[1]),
//                        CV_8UC3, (unsigned char*)buf.ptr);
                auto mat = PyArrayToCvMat(np);
                OCRResult oret;
                if(!self.infer(mat, &oret)){
                    fprintf(stderr, "[OcrInfer]: infer failed.\n");
                }
                return oret;
            })
            .def("inferBatch", [](OcrApi& self, std::vector<pybind11::array>& nps){
                std::vector<cv::Mat> imgs;
                for(int i = 0 ; i < (int)nps.size() ; ++i){
                    imgs.push_back(PyArrayToCvMat(nps[i]));
                }
                std::vector<OCRResult> orets;
                if(!self.inferBatch(imgs, &orets)){
                    fprintf(stderr, "[OcrInfer]: inferBatch failed.\n");
                }
                return orets;
            })
            ;
}

}

