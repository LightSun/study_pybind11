#pragma once

#include "pybind11/numpy.h"
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/eval.h>

namespace h7 {

void BindEDM(pybind11::module&);
void BindOcrInfer(pybind11::module&);

}
