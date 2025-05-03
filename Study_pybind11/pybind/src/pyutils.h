#pragma once

#include "pybind11/numpy.h"
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/eval.h>
#include "opencv2/opencv.hpp"
#include "infer/src/ocr_common.h"

namespace h7 {

static inline int NumpyDataTypeToOpenCvTypeV2(pybind11::array& pyarray) {
  if (pybind11::isinstance<pybind11::array_t<std::int32_t>>(pyarray)) {
    return CV_32S;
  } else if (pybind11::isinstance<pybind11::array_t<std::int8_t>>(pyarray)) {
    return CV_8S;
  } else if (pybind11::isinstance<pybind11::array_t<std::uint8_t>>(pyarray)) {
    return CV_8U;
  } else if (pybind11::isinstance<pybind11::array_t<std::float_t>>(pyarray)) {
    return CV_32F;
  } else {
    FDASSERT(
        false,
        "NumpyDataTypeToOpenCvTypeV2() only support int32/int8/uint8/float32 "
        "now.");
  }
  return CV_8U;
}

static inline cv::Mat PyArrayToCvMat(pybind11::array& pyarray) {
  // auto cv_type = NumpyDataTypeToOpenCvType(pyarray.dtype());
  auto cv_type = NumpyDataTypeToOpenCvTypeV2(pyarray);
  FDASSERT(
      pyarray.ndim() == 3,
      "Require rank of array to be 3 with HWC format while converting it to "
      "cv::Mat.");
  int channel = *(pyarray.shape() + 2);
  int height = *(pyarray.shape());
  int width = *(pyarray.shape() + 1);
  return cv::Mat(height, width, CV_MAKETYPE(cv_type, channel),
                 pyarray.mutable_data());
}
}
