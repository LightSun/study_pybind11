#pragma once

#include <string>
#include <iostream>

namespace fastdeploy {

enum FDDataType{
  BOOL,
  INT16,
  INT32,
  INT64,
  FP16,
  FP32,
  FP64,

  UINT8,
  INT8
};

static inline std::string Str(const FDDataType& fdt) {
  std::string out;
  switch (fdt) {
    case FDDataType::BOOL:
      out = "FDDataType::BOOL";
      break;
    case FDDataType::INT16:
      out = "FDDataType::INT16";
      break;
    case FDDataType::INT32:
      out = "FDDataType::INT32";
      break;
    case FDDataType::INT64:
      out = "FDDataType::INT64";
      break;
    case FDDataType::FP32:
      out = "FDDataType::FP32";
      break;
    case FDDataType::FP64:
      out = "FDDataType::FP64";
      break;
    case FDDataType::FP16:
      out = "FDDataType::FP16";
      break;
    case FDDataType::UINT8:
      out = "FDDataType::UINT8";
      break;
    case FDDataType::INT8:
      out = "FDDataType::INT8";
      break;
    default:
      out = "FDDataType::UNKNOWN";
  }
  return out;
}

/*
std::ostream& operator<<(std::ostream& out, const FDDataType& fdt) {
  switch (fdt) {
    case FDDataType::BOOL:
      out << "FDDataType::BOOL";
      break;
    case FDDataType::INT16:
      out << "FDDataType::INT16";
      break;
    case FDDataType::INT32:
      out << "FDDataType::INT32";
      break;
    case FDDataType::INT64:
      out << "FDDataType::INT64";
      break;
    case FDDataType::FP32:
      out << "FDDataType::FP32";
      break;
    case FDDataType::FP64:
      out << "FDDataType::FP64";
      break;
    case FDDataType::FP16:
      out << "FDDataType::FP16";
      break;
    case FDDataType::UINT8:
      out << "FDDataType::UINT8";
      break;
    case FDDataType::INT8:
      out << "FDDataType::INT8";
      break;
    default:
      out << "FDDataType::UNKNOWN";
  }
  return out;
}
*/

//template <typename PlainType>
//const FDDataType TypeToDataType<PlainType>::dtype = UNKNOWN1;

//template <>
//const FDDataType TypeToDataType<bool>::dtype = BOOL;

//template <>
//const FDDataType TypeToDataType<int16_t>::dtype = INT16;

//template <>
//const FDDataType TypeToDataType<int32_t>::dtype = INT32;

//template <>
//const FDDataType TypeToDataType<int64_t>::dtype = INT64;

//template <>
//const FDDataType TypeToDataType<float>::dtype = FP32;

//template <>
//const FDDataType TypeToDataType<double>::dtype = FP64;

//template <>
//const FDDataType TypeToDataType<uint8_t>::dtype = UINT8;

//template <>
//const FDDataType TypeToDataType<int8_t>::dtype = INT8;

}
