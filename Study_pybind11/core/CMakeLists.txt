
#include_directories(./)

## lib
file(GLOB_RECURSE SRC_MODEL_M
    src/*.h
    src/*.hpp
    src/*.cpp
    src/*.c
    )

add_library(MedCommonLib
    ${SRC_MODEL_M}
    )
target_compile_definitions(MedCommonLib PUBLIC DISABLE_ONNX2TRT)
target_link_libraries(MedCommonLib PUBLIC
    snappy
    )
if(WIN32)
    target_link_libraries(MedCommonLib PUBLIC shlwapi)
endif(WIN32)


