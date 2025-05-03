
#
include_directories(${ModelM_DIR}/include)
#
#set(FD_EXT_LIBS ${FD_LIB_DIR}/libpaddle_inference.so
#    ${FD_LIB_DIR}/libopenvino.so.2300
#    ${FD_LIB_DIR}/libcore_tokenizers.so
#    ${FD_LIB_DIR}/libpaddle2onnx.so.1.0.8rc
#    )
#
file(GLOB_RECURSE SRC_PYBIND
    src/*.h
    src/*.hpp
    src/*.cpp
    src/*.c
    src/*.cc
    )

#add_library(PyOcrInfer MODULE
#    ${SRC_PYBIND}
#    )
pybind11_add_module(PyOcrInfer
    ${SRC_PYBIND}
    )
set(PyOcr_VERSION_INFO "1.0.0")
target_compile_definitions(PyOcrInfer PRIVATE VERSION_INFO=${PyOcr_VERSION_INFO})
target_include_directories(PyOcrInfer PUBLIC ${PY_BIND11_DIR}/include)
set_target_properties(PyOcrInfer
  PROPERTIES
    PREFIX "${PYTHON_MODULE_PREFIX}"
    SUFFIX "${PYTHON_MODULE_EXTENSION}"
  )
target_link_libraries(PyOcrInfer PUBLIC OcrInfer
    pybind11::module
    )
#target_link_libraries(PyOcrInfer PUBLIC ${FD_EXT_LIBS})

