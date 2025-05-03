#pragma once

#include "opencv2/core.hpp"
#include "infer/src/ocr_common.h"
#include "infer/src/ocr_config.h"

namespace h7 {

class INoCopy
{
protected:
    INoCopy() = default;
    virtual ~INoCopy() = default;
    INoCopy(INoCopy const& other) = delete;
    INoCopy& operator=(INoCopy const& other) = delete;
    INoCopy(INoCopy&& other) = delete;
    INoCopy& operator=(INoCopy&& other) = delete;
};

class FD_InferBase{
public:
    ~FD_InferBase(){}

    void setOcrConfig(const OcrConfig& in){
        m_config = in;
    }
protected:
    OcrConfig m_config;
};
}
