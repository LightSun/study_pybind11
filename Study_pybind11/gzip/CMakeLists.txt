

add_subdirectory(zlib-1.3.1/)
#
file(GLOB_RECURSE GZIP_SRC_CORE
    src/*.h src/*.cpp)

add_library(h7zip
    ${GZIP_SRC_CORE}
    )

target_link_libraries(h7zip PUBLIC zlib MedCommonLib)

## test

file(GLOB_RECURSE TEST_GZIP_SRC_CORE
    main/*.h main/*.cpp
    )
add_executable(test_gzip
    ${TEST_GZIP_SRC_CORE}
)

target_link_libraries(test_gzip PUBLIC ${ABSL_TARGETS}
    h7zip
    MedCommonLib
    pthread
    )
