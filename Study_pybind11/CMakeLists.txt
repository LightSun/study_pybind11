cmake_minimum_required(VERSION 3.10)

project(Study_pybind11 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

SET(CMAKE_POSITION_INDEPENDENT_CODE ON)
if(MSVC)
        add_compile_definitions(USE_MSC_ATOMICS)
    else(MSVC)
        add_compile_definitions(USE_C11_ATOMICS)
    endif(MSVC)
##
include(cmake/ENV.cmake)
##
include_directories(./)
SET(PY_BIND11_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/pybind11)
SET(ModelM_libs ${ModelM_DIR}/lib/libModelM_shared.so)

#fd
find_package(FastDeploy)
include_directories(${FastDeploy_INCLUDE_DIRS})
##
#add_subdirectory(infer/)
add_subdirectory(snappy/)
add_subdirectory(core/)
add_subdirectory(gzip/)
add_subdirectory(third_party/pybind11)
#add_subdirectory(pybind/)
#add_subdirectory(http_service/)
add_subdirectory(PyEncDec/)
add_subdirectory(third_party/lua-5.3.6)

