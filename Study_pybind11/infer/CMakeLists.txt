
# opencv
find_package(OpenCV 4 REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

message(STATUS "OpenCV library status:")
message(STATUS "    version: ${OpenCV_VERSION}")
message(STATUS "    libraries: ${OpenCV_LIBS}")
message(STATUS "    config: ${OpenCV_DIR}")
message(STATUS "    include path: ${OpenCV_INCLUDE_DIRS}")

## MM
include_directories(${ModelM_DIR}/include)

## lib
file(GLOB_RECURSE SRC_LIB_OCR
    src/*.h
    src/*.hpp
    src/*.cpp
    src/*.c
    )
add_library(OcrInfer
    ${SRC_LIB_OCR}
    )

target_link_libraries(OcrInfer PUBLIC
    ${FastDeploy_LIBS}
    ${ModelM_libs}
    ${OpenCV_LIBS}
    )

## lib_shared
add_library(OcrInfer_shared SHARED
    ${SRC_LIB_OCR}
    )

target_link_libraries(OcrInfer_shared PUBLIC
    ${FastDeploy_LIBS}
    ${ModelM_libs}
    ${OpenCV_LIBS}
    )

## rapid
file(GLOB_RECURSE SRC_RAPID_OCR
    src_rapid/*.h
    src_rapid/*.hpp
    src_rapid/*.cpp
    src_rapid/*.c
    )

add_library(MedOcrInfer SHARED
    ${SRC_RAPID_OCR}
    src/EDMHelper.h
    src/ocr_common.h
    src/ocr_config.h
    src/fd_ocr.h
    src/utils.h
    src/utils.cpp
    )
target_include_directories(MedOcrInfer PRIVATE
    ${RAPID_OCR_DIR}/include
    )
target_link_libraries(MedOcrInfer PRIVATE
    ${ModelM_libs}
    ${OpenCV_LIBS}
    ${RAPID_OCR_DIR}/lib/libRapidOcrOnnx.so
    )

## ocr_test
add_executable(MedOcrInferTest
    ${SRC_RAPID_OCR}
    src/EDMHelper.h
    src/ocr_common.h
    src/ocr_config.h
    src/fd_ocr.h
    src/utils.h
    src/utils.cpp
    test_rapid/main.cpp
    )
target_include_directories(MedOcrInferTest PRIVATE
    ${RAPID_OCR_DIR}/include
    )
target_link_libraries(MedOcrInferTest PRIVATE
    ${ModelM_libs}
    ${OpenCV_LIBS}
    ${RAPID_OCR_DIR}/lib/libRapidOcrOnnx.so
    )

## test
file(GLOB_RECURSE SRC_OCR_TEST
    main/*.h
    main/*.hpp
    main/*.cpp
    )
add_executable(OcrTest
    ${SRC_OCR_TEST}
    )
target_link_libraries(OcrTest PUBLIC
    OcrInfer
    )

## testWraper
file(GLOB_RECURSE SRC_TEST_WRAPPER
    mainWrap/*.h
    mainWrap/*.hpp
    mainWrap/*.cpp
    )
add_executable(OcrTestWrap
    ${SRC_TEST_WRAPPER}
    )
