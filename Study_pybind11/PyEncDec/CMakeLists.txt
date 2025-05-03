
set(EncDec_DIR /home/heaven7/heaven7/env/linux/EncDec)

file(GLOB_RECURSE SRC_ENC_DEC
    src/*.h
    src/*.hpp
    src/*.cpp
    src/*.c
    )

#add_library(PyEncDec
#    ${SRC_ENC_DEC}
#    )
#
pybind11_add_module(PyEncDec
    ${SRC_ENC_DEC}
    )
set(PyEncDec_VERSION_INFO "1.0.0")
target_compile_definitions(PyEncDec PRIVATE VERSION_INFO=${PyEncDec_VERSION_INFO})
target_include_directories(PyEncDec PUBLIC ${PY_BIND11_DIR}/include)
set_target_properties(PyEncDec
  PROPERTIES
    PREFIX "${PYTHON_MODULE_PREFIX}"
    SUFFIX "${PYTHON_MODULE_EXTENSION}"
  )
target_link_libraries(PyEncDec PUBLIC pybind11::module)
#---
target_include_directories(PyEncDec PUBLIC ${EncDec_DIR}/include)
target_link_libraries(PyEncDec PUBLIC
    h7zip pthread
    ${EncDec_DIR}/lib/libMedEncDec0.so
    )

##
file(GLOB_RECURSE SRC_ENC_PY
    main/*.h
    main/*.hpp
    main/*.cpp
    main/*.c
    )
add_executable(MedEncToolPy
    ${SRC_ENC_PY}
    )
target_include_directories(MedEncToolPy PUBLIC ${EncDec_DIR}/include)
target_link_libraries(MedEncToolPy PUBLIC
    h7zip pthread
    ${EncDec_DIR}/lib/libMedEncDec0.so
    )
