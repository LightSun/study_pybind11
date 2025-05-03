#pragma once

#include <stdexcept>
#include "macro.h"

BIND11_NAMESPACE_BEGIN(BIND11_NAMESPACE)

#if defined(_MSC_VER)
#    pragma warning(push)
#    pragma warning(disable : 4275)
//     warning C4275: An exported class was derived from a class that wasn't exported.
//     Can be ignored when derived from a STL class.
#endif
/// C++ bindings of builtin Python exceptions
class BIND11_EXPORT_EXCEPTION builtin_exception : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
    /// Set the error using the Python C API
    virtual void set_error() const = 0;
};
#if defined(_MSC_VER)
#    pragma warning(pop)
#endif

#define BIND11_RUNTIME_EXCEPTION(name, type)                                                    \
    class BIND11_EXPORT_EXCEPTION name : public builtin_exception {                             \
    public:                                                                                       \
        using builtin_exception::builtin_exception;                                               \
        name() : name(#type) {}                                                                      \
        void set_error() const override { /*PyErr_SetString(type, what()); */ }                        \
    };

BIND11_RUNTIME_EXCEPTION(stop_iteration, PyExc_StopIteration)
BIND11_RUNTIME_EXCEPTION(index_error, PyExc_IndexError)
BIND11_RUNTIME_EXCEPTION(key_error, PyExc_KeyError)
BIND11_RUNTIME_EXCEPTION(value_error, PyExc_ValueError)
BIND11_RUNTIME_EXCEPTION(type_error, PyExc_TypeError)
BIND11_RUNTIME_EXCEPTION(buffer_error, PyExc_BufferError)
BIND11_RUNTIME_EXCEPTION(import_error, PyExc_ImportError)
BIND11_RUNTIME_EXCEPTION(attribute_error, PyExc_AttributeError)
BIND11_RUNTIME_EXCEPTION(cast_error, PyExc_RuntimeError) /// Thrown when BIND11::cast or
/// handle::call fail due to a type
/// casting error
BIND11_RUNTIME_EXCEPTION(reference_cast_error, PyExc_RuntimeError) /// Used internally

[[noreturn]] BIND11_NOINLINE void BIND11_fail(const char *reason) {
    //assert(!PyErr_Occurred());
    throw std::runtime_error(reason);
}
[[noreturn]] BIND11_NOINLINE void BIND11_fail(const std::string &reason) {
    //assert(!PyErr_Occurred());
    throw std::runtime_error(reason);
}
BIND11_NAMESPACE_END(BIND11_NAMESPACE)
