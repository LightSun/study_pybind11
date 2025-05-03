
find_package(OpenSSL REQUIRED)
set(HTTP_DEP_LIBS
    OpenSSL::Crypto OpenSSL::SSL  # modern
    pthread
    #${OPENSSL_SSL_LIBRARY} ${OPENSSL_CRYPTO_LIBRARY}  # old style
)

# hv
#SET(LIB_HV_DIR /home/heaven7/heaven7/env/linux/libhv-1.3.2-ssl)
SET(LIB_HV_LIB ${LIB_HV_DIR}/lib/libhv_static.a)
include_directories(${LIB_HV_DIR}/include)


## main
file(GLOB_RECURSE SRC_HTTP_SERVICE
    src/*.h
    src/*.cpp
    )

add_executable(HttpOcrInfer
    ${SRC_HTTP_SERVICE}
    )
target_include_directories(HttpOcrInfer PUBLIC ${ModelM_DIR}/include)
target_link_libraries(HttpOcrInfer PUBLIC
    ${LIB_HV_LIB}
    ${HTTP_DEP_LIBS}
    OcrInfer
    )
