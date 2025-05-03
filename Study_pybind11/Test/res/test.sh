

DIR="/media/heaven7/Elements_SE/study/work/OCR/compiled_fastdeploy_sdk"
FD_INSDIR=${DIR}/third_libs/install
#
LIB_TRT_DIR=${FD_INSDIR}/tensorrt/lib
LIB_PADDLE_DIR=${FD_INSDIR}/paddle_inference/paddle/lib
LIB_PAD2OX_DIR=${FD_INSDIR}/paddle2onnx/lib
LIB_OPENVINO_DIR=${FD_INSDIR}/openvino/runtime/lib
LIB_TOKEN_DIR=${FD_INSDIR}/fast_tokenizer/lib

OPENCV_DIR=/home/heaven7/heaven7/libs/opencv-3.4.7/opencv-4.5.4/_install/lib64

export LD_LIBRARY_PATH=${LIB_TRT_DIR}:${LIB_PADDLE_DIR}:${LIB_PAD2OX_DIR}:${LIB_OPENVINO_DIR}:${LIB_TOKEN_DIR}:${OPENCV_DIR}

./OcrTest

