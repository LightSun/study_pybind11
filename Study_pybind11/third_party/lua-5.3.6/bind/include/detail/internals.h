/*
    BIND11/detail/internals.h: Internal data structure and related functions

    Copyright (c) 2017 Wenzel Jakob <wenzel.jakob@epfl.ch>

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/

#pragma once

//#include "../pytypes.h"

#include "common.h"
#include <exception>

/// Tracks the `internals` and `type_info` ABI version independent of the main library version.
///
/// Some portions of the code use an ABI that is conditional depending on this
/// version number.  That allows ABI-breaking changes to be "pre-implemented".
/// Once the default version number is incremented, the conditional logic that
/// no longer applies can be removed.  Additionally, users that need not
/// maintain ABI compatibility can increase the version number in order to take
/// advantage of any functionality/efficiency improvements that depend on the
/// newer ABI.
///
/// WARNING: If you choose to manually increase the ABI version, note that
/// BIND11 may not be tested as thoroughly with a non-default ABI version, and
/// further ABI-incompatible changes may be made before the ABI is officially
/// changed to the new version.
#ifndef BIND11_INTERNALS_VERSION
#    define BIND11_INTERNALS_VERSION 4
#endif

BIND11_NAMESPACE_BEGIN(BIND11_NAMESPACE)

using ExceptionTranslator = void (*)(std::exception_ptr);

BIND11_NAMESPACE_BEGIN(detail)

// Forward declarations
//inline TypeObject *make_static_property_type();
//inline TypeObject *make_default_metaclass();
//inline Object *make_object_base_type(TypeObject *metaclass);

// The old Python Thread Local Storage (TLS) API is deprecated in Python 3.7 in favor of the new
// Thread Specific Storage (TSS) API.
#if PY_VERSION_HEX >= 0x03070000
// Avoid unnecessary allocation of `Py_tss_t`, since we cannot use
// `Py_LIMITED_API` anyway.
#    if BIND11_INTERNALS_VERSION > 4
#        define BIND11_TLS_KEY_REF Py_tss_t &
#        ifdef __GNUC__
// Clang on macOS warns due to `Py_tss_NEEDS_INIT` not specifying an initializer
// for every field.
#            define BIND11_TLS_KEY_INIT(var)                                                    \
                _Pragma("GCC diagnostic push")                                         /**/       \
                    _Pragma("GCC diagnostic ignored \"-Wmissing-field-initializers\"") /**/       \
                    Py_tss_t var                                                                  \
                    = Py_tss_NEEDS_INIT;                                                          \
                _Pragma("GCC diagnostic pop")
#        else
#            define BIND11_TLS_KEY_INIT(var) Py_tss_t var = Py_tss_NEEDS_INIT;
#        endif
#        define BIND11_TLS_KEY_CREATE(var) (PyThread_tss_create(&(var)) == 0)
#        define BIND11_TLS_GET_VALUE(key) PyThread_tss_get(&(key))
#        define BIND11_TLS_REPLACE_VALUE(key, value) PyThread_tss_set(&(key), (value))
#        define BIND11_TLS_DELETE_VALUE(key) PyThread_tss_set(&(key), nullptr)
#        define BIND11_TLS_FREE(key) PyThread_tss_delete(&(key))
#    else
#        define BIND11_TLS_KEY_REF Py_tss_t *
#        define BIND11_TLS_KEY_INIT(var) Py_tss_t *var = nullptr;
#        define BIND11_TLS_KEY_CREATE(var)                                                      \
            (((var) = PyThread_tss_alloc()) != nullptr && (PyThread_tss_create((var)) == 0))
#        define BIND11_TLS_GET_VALUE(key) PyThread_tss_get((key))
#        define BIND11_TLS_REPLACE_VALUE(key, value) PyThread_tss_set((key), (value))
#        define BIND11_TLS_DELETE_VALUE(key) PyThread_tss_set((key), nullptr)
#        define BIND11_TLS_FREE(key) PyThread_tss_free(key)
#    endif
#else
// Usually an int but a long on Cygwin64 with Python 3.x
#    define BIND11_TLS_KEY_REF decltype(PyThread_create_key())
#    define BIND11_TLS_KEY_INIT(var) BIND11_TLS_KEY_REF var = 0;
#    define BIND11_TLS_KEY_CREATE(var) (((var) = PyThread_create_key()) != -1)
#    define BIND11_TLS_GET_VALUE(key) PyThread_get_key_value((key))
#    if defined(PYPY_VERSION)
// On CPython < 3.4 and on PyPy, `PyThread_set_key_value` strangely does not set
// the value if it has already been set.  Instead, it must first be deleted and
// then set again.
inline void tls_replace_value(BIND11_TLS_KEY_REF key, void *value) {
    PyThread_delete_key_value(key);
    PyThread_set_key_value(key, value);
}
#        define BIND11_TLS_DELETE_VALUE(key) PyThread_delete_key_value(key)
#        define BIND11_TLS_REPLACE_VALUE(key, value)                                            \
            ::BIND11::detail::tls_replace_value((key), (value))
#    else
#        define BIND11_TLS_DELETE_VALUE(key) PyThread_set_key_value((key), nullptr)
#        define BIND11_TLS_REPLACE_VALUE(key, value) PyThread_set_key_value((key), (value))
#    endif
#    define BIND11_TLS_FREE(key) (void) key
#endif

// Python loads modules by default with dlopen with the RTLD_LOCAL flag; under libc++ and possibly
// other STLs, this means `typeid(A)` from one module won't equal `typeid(A)` from another module
// even when `A` is the same, non-hidden-visibility type (e.g. from a common include).  Under
// libstdc++, this doesn't happen: equality and the type_index hash are based on the type name,
// which works.  If not under a known-good stl, provide our own name-based hash and equality
// functions that use the type name.
#if defined(__GLIBCXX__)
inline bool same_type(const std::type_info &lhs, const std::type_info &rhs) { return lhs == rhs; }
using type_hash = std::hash<std::type_index>;
using type_equal_to = std::equal_to<std::type_index>;
#else
inline bool same_type(const std::type_info &lhs, const std::type_info &rhs) {
    return lhs.name() == rhs.name() || std::strcmp(lhs.name(), rhs.name()) == 0;
}

struct type_hash {
    size_t operator()(const std::type_index &t) const {
        size_t hash = 5381;
        const char *ptr = t.name();
        while (auto c = static_cast<unsigned char>(*ptr++)) {
            hash = (hash * 33) ^ c;
        }
        return hash;
    }
};

struct type_equal_to {
    bool operator()(const std::type_index &lhs, const std::type_index &rhs) const {
        return lhs.name() == rhs.name() || std::strcmp(lhs.name(), rhs.name()) == 0;
    }
};
#endif

template <typename value_type>
using type_map = std::unordered_map<std::type_index, value_type, type_hash, type_equal_to>;

struct override_hash {
    inline size_t operator()(const std::pair<const Object *, const char *> &v) const {
        size_t value = std::hash<const void *>()(v.first);
        value ^= std::hash<const void *>()(v.second) + 0x9e3779b9 + (value << 6) + (value >> 2);
        return value;
    }
};

/// Internal data structure used to track registered instances and types.
/// Whenever binary incompatible changes are made to this structure,
/// `BIND11_INTERNALS_VERSION` must be incremented.
struct internals {
    // std::type_index -> BIND11's type information
    type_map<type_info *> registered_types_cpp;
    // TypeObject* -> base type_info(s)
    std::unordered_map<TypeObject *, std::vector<type_info *>> registered_types_py;
    std::unordered_multimap<const void *, instance *> registered_instances; // void * -> instance*
    std::unordered_set<std::pair<const Object *, const char *>, override_hash>
        inactive_override_cache;
    type_map<std::vector<bool (*)(Object *, void *&)>> direct_conversions;
    std::unordered_map<const Object *, std::vector<Object *>> patients;
    std::forward_list<ExceptionTranslator> registered_exception_translators;
    std::unordered_map<std::string, void *> shared_data; // Custom data to be shared across
                                                         // extensions
#if BIND11_INTERNALS_VERSION == 4
    std::vector<Object *> unused_loader_patient_stack_remove_at_v5;
#endif
    std::forward_list<std::string> static_strings; // Stores the std::strings backing
                                                   // detail::c_str()
    TypeObject *static_property_type;
    TypeObject *default_metaclass;
    Object *instance_base;
#if defined(WITH_THREAD)
    BIND11_TLS_KEY_INIT(tstate)
#    if BIND11_INTERNALS_VERSION > 4
    BIND11_TLS_KEY_INIT(loader_life_support_tls_key)
#    endif // BIND11_INTERNALS_VERSION > 4
    PyInterpreterState *istate = nullptr;
    ~internals() {
#    if BIND11_INTERNALS_VERSION > 4
        BIND11_TLS_FREE(loader_life_support_tls_key);
#    endif // BIND11_INTERNALS_VERSION > 4

        // This destructor is called *after* Py_Finalize() in finalize_interpreter().
        // That *SHOULD BE* fine. The following details what happens when PyThread_tss_free is
        // called. BIND11_TLS_FREE is PyThread_tss_free on python 3.7+. On older python, it does
        // nothing. PyThread_tss_free calls PyThread_tss_delete and PyMem_RawFree.
        // PyThread_tss_delete just calls TlsFree (on Windows) or pthread_key_delete (on *NIX).
        // Neither of those have anything to do with CPython internals. PyMem_RawFree *requires*
        // that the `tstate` be allocated with the CPython allocator.
        BIND11_TLS_FREE(tstate);
    }
#endif
};

/// Additional type information which does not fit into the TypeObject.
/// Changes to this struct also require bumping `BIND11_INTERNALS_VERSION`.
struct type_info {
    TypeObject *type;
    const std::type_info *cpptype;
    size_t type_size, type_align, holder_size_in_ptrs;
    void *(*operator_new)(size_t);
    void (*init_instance)(instance *, const void *);
    void (*dealloc)(value_and_holder &v_h);
    std::vector<Object *(*) (Object *, TypeObject *)> implicit_conversions;
    std::vector<std::pair<const std::type_info *, void *(*) (void *)>> implicit_casts;
    std::vector<bool (*)(Object *, void *&)> *direct_conversions;
    buffer_info *(*get_buffer)(Object *, void *) = nullptr;
    void *get_buffer_data = nullptr;
    void *(*module_local_load)(Object *, const type_info *) = nullptr;
    /* A simple type never occurs as a (direct or indirect) parent
     * of a class that makes use of multiple inheritance.
     * A type can be simple even if it has non-simple ancestors as long as it has no descendants.
     */
    bool simple_type : 1;
    /* True if there is no multiple inheritance in this type's inheritance tree */
    bool simple_ancestors : 1;
    /* for base vs derived holder_type checks */
    bool default_holder : 1;
    /* true if this is a type registered with py::module_local */
    bool module_local : 1;
};

/// On MSVC, debug and release builds are not ABI-compatible!
#if defined(_MSC_VER) && defined(_DEBUG)
#    define BIND11_BUILD_TYPE "_debug"
#else
#    define BIND11_BUILD_TYPE ""
#endif

/// Let's assume that different compilers are ABI-incompatible.
/// A user can manually set this string if they know their
/// compiler is compatible.
#ifndef BIND11_COMPILER_TYPE
#    if defined(_MSC_VER)
#        define BIND11_COMPILER_TYPE "_msvc"
#    elif defined(__INTEL_COMPILER)
#        define BIND11_COMPILER_TYPE "_icc"
#    elif defined(__clang__)
#        define BIND11_COMPILER_TYPE "_clang"
#    elif defined(__PGI)
#        define BIND11_COMPILER_TYPE "_pgi"
#    elif defined(__MINGW32__)
#        define BIND11_COMPILER_TYPE "_mingw"
#    elif defined(__CYGWIN__)
#        define BIND11_COMPILER_TYPE "_gcc_cygwin"
#    elif defined(__GNUC__)
#        define BIND11_COMPILER_TYPE "_gcc"
#    else
#        define BIND11_COMPILER_TYPE "_unknown"
#    endif
#endif

/// Also standard libs
#ifndef BIND11_STDLIB
#    if defined(_LIBCPP_VERSION)
#        define BIND11_STDLIB "_libcpp"
#    elif defined(__GLIBCXX__) || defined(__GLIBCPP__)
#        define BIND11_STDLIB "_libstdcpp"
#    else
#        define BIND11_STDLIB ""
#    endif
#endif

/// On Linux/OSX, changes in __GXX_ABI_VERSION__ indicate ABI incompatibility.
#ifndef BIND11_BUILD_ABI
#    if defined(__GXX_ABI_VERSION)
#        define BIND11_BUILD_ABI "_cxxabi" BIND11_TOSTRING(__GXX_ABI_VERSION)
#    else
#        define BIND11_BUILD_ABI ""
#    endif
#endif

#ifndef BIND11_INTERNALS_KIND
#    if defined(WITH_THREAD)
#        define BIND11_INTERNALS_KIND ""
#    else
#        define BIND11_INTERNALS_KIND "_without_thread"
#    endif
#endif

#define BIND11_INTERNALS_ID                                                                     \
    "__BIND11_internals_v" BIND11_TOSTRING(BIND11_INTERNALS_VERSION)                        \
        BIND11_INTERNALS_KIND BIND11_COMPILER_TYPE BIND11_STDLIB BIND11_BUILD_ABI         \
            BIND11_BUILD_TYPE "__"

#define BIND11_MODULE_LOCAL_ID                                                                  \
    "__BIND11_module_local_v" BIND11_TOSTRING(BIND11_INTERNALS_VERSION)                     \
        BIND11_INTERNALS_KIND BIND11_COMPILER_TYPE BIND11_STDLIB BIND11_BUILD_ABI         \
            BIND11_BUILD_TYPE "__"

/// Each module locally stores a pointer to the `internals` data. The data
/// itself is shared among modules with the same `BIND11_INTERNALS_ID`.
inline internals **&get_internals_pp() {
    static internals **internals_pp = nullptr;
    return internals_pp;
}

// forward decl
inline void translate_exception(std::exception_ptr);

template <class T,
          enable_if_t<std::is_same<std::nested_exception, remove_cvref_t<T>>::value, int> = 0>
bool handle_nested_exception(const T &exc, const std::exception_ptr &p) {
    std::exception_ptr nested = exc.nested_ptr();
    if (nested != nullptr && nested != p) {
        translate_exception(nested);
        return true;
    }
    return false;
}

template <class T,
          enable_if_t<!std::is_same<std::nested_exception, remove_cvref_t<T>>::value, int> = 0>
bool handle_nested_exception(const T &exc, const std::exception_ptr &p) {
    if (const auto *nep = dynamic_cast<const std::nested_exception *>(std::addressof(exc))) {
        return handle_nested_exception(*nep, p);
    }
    return false;
}

inline bool raise_err(Object *exc_type, const char *msg) {
    if (PyErr_Occurred()) {
        raise_from(exc_type, msg);
        return true;
    }
    PyErr_SetString(exc_type, msg);
    return false;
}

inline void translate_exception(std::exception_ptr p) {
    if (!p) {
        return;
    }
    try {
        std::rethrow_exception(p);
    } catch (error_already_set &e) {
        handle_nested_exception(e, p);
        e.restore();
        return;
    } catch (const builtin_exception &e) {
        // Could not use template since it's an abstract class.
        if (const auto *nep = dynamic_cast<const std::nested_exception *>(std::addressof(e))) {
            handle_nested_exception(*nep, p);
        }
        e.set_error();
        return;
    } catch (const std::bad_alloc &e) {
        handle_nested_exception(e, p);
        raise_err(PyExc_MemoryError, e.what());
        return;
    } catch (const std::domain_error &e) {
        handle_nested_exception(e, p);
        raise_err(PyExc_ValueError, e.what());
        return;
    } catch (const std::invalid_argument &e) {
        handle_nested_exception(e, p);
        raise_err(PyExc_ValueError, e.what());
        return;
    } catch (const std::length_error &e) {
        handle_nested_exception(e, p);
        raise_err(PyExc_ValueError, e.what());
        return;
    } catch (const std::out_of_range &e) {
        handle_nested_exception(e, p);
        raise_err(PyExc_IndexError, e.what());
        return;
    } catch (const std::range_error &e) {
        handle_nested_exception(e, p);
        raise_err(PyExc_ValueError, e.what());
        return;
    } catch (const std::overflow_error &e) {
        handle_nested_exception(e, p);
        raise_err(PyExc_OverflowError, e.what());
        return;
    } catch (const std::exception &e) {
        handle_nested_exception(e, p);
        raise_err(PyExc_RuntimeError, e.what());
        return;
    } catch (const std::nested_exception &e) {
        handle_nested_exception(e, p);
        raise_err(PyExc_RuntimeError, "Caught an unknown nested exception!");
        return;
    } catch (...) {
        raise_err(PyExc_RuntimeError, "Caught an unknown exception!");
        return;
    }
}

#if !defined(__GLIBCXX__)
inline void translate_local_exception(std::exception_ptr p) {
    try {
        if (p) {
            std::rethrow_exception(p);
        }
    } catch (error_already_set &e) {
        e.restore();
        return;
    } catch (const builtin_exception &e) {
        e.set_error();
        return;
    }
}
#endif

/// Return a reference to the current `internals` data
BIND11_NOINLINE internals &get_internals() {
    auto **&internals_pp = get_internals_pp();
    if (internals_pp && *internals_pp) {
        return **internals_pp;
    }

    // Ensure that the GIL is held since we will need to make Python calls.
    // Cannot use py::gil_scoped_acquire here since that constructor calls get_internals.
    struct gil_scoped_acquire_local {
        gil_scoped_acquire_local() : state(PyGILState_Ensure()) {}
        ~gil_scoped_acquire_local() { PyGILState_Release(state); }
        const PyGILState_STATE state;
    } gil;

    BIND11_STR_TYPE id(BIND11_INTERNALS_ID);
    auto builtins = handle(PyEval_GetBuiltins());
    if (builtins.contains(id) && isinstance<capsule>(builtins[id])) {
        internals_pp = static_cast<internals **>(capsule(builtins[id]));

        // We loaded builtins through python's builtins, which means that our `error_already_set`
        // and `builtin_exception` may be different local classes than the ones set up in the
        // initial exception translator, below, so add another for our local exception classes.
        //
        // libstdc++ doesn't require this (types there are identified only by name)
        // libc++ with CPython doesn't require this (types are explicitly exported)
        // libc++ with PyPy still need it, awaiting further investigation
#if !defined(__GLIBCXX__)
        (*internals_pp)->registered_exception_translators.push_front(&translate_local_exception);
#endif
    } else {
        if (!internals_pp) {
            internals_pp = new internals *();
        }
        auto *&internals_ptr = *internals_pp;
        internals_ptr = new internals();
#if defined(WITH_THREAD)

#    if PY_VERSION_HEX < 0x03090000
        PyEval_InitThreads();
#    endif
        PyThreadState *tstate = PyThreadState_Get();
        if (!BIND11_TLS_KEY_CREATE(internals_ptr->tstate)) {
            BIND11_fail("get_internals: could not successfully initialize the tstate TSS key!");
        }
        BIND11_TLS_REPLACE_VALUE(internals_ptr->tstate, tstate);

#    if BIND11_INTERNALS_VERSION > 4
        if (!BIND11_TLS_KEY_CREATE(internals_ptr->loader_life_support_tls_key)) {
            BIND11_fail("get_internals: could not successfully initialize the "
                          "loader_life_support TSS key!");
        }
#    endif
        internals_ptr->istate = tstate->interp;
#endif
        builtins[id] = capsule(internals_pp);
        internals_ptr->registered_exception_translators.push_front(&translate_exception);
        internals_ptr->static_property_type = make_static_property_type();
        internals_ptr->default_metaclass = make_default_metaclass();
        internals_ptr->instance_base = make_object_base_type(internals_ptr->default_metaclass);
    }
    return **internals_pp;
}

// the internals struct (above) is shared between all the modules. local_internals are only
// for a single module. Any changes made to internals may require an update to
// BIND11_INTERNALS_VERSION, breaking backwards compatibility. local_internals is, by design,
// restricted to a single module. Whether a module has local internals or not should not
// impact any other modules, because the only things accessing the local internals is the
// module that contains them.
struct local_internals {
    type_map<type_info *> registered_types_cpp;
    std::forward_list<ExceptionTranslator> registered_exception_translators;
#if defined(WITH_THREAD) && BIND11_INTERNALS_VERSION == 4

    // For ABI compatibility, we can't store the loader_life_support TLS key in
    // the `internals` struct directly.  Instead, we store it in `shared_data` and
    // cache a copy in `local_internals`.  If we allocated a separate TLS key for
    // each instance of `local_internals`, we could end up allocating hundreds of
    // TLS keys if hundreds of different BIND11 modules are loaded (which is a
    // plausible number).
    BIND11_TLS_KEY_INIT(loader_life_support_tls_key)

    // Holds the shared TLS key for the loader_life_support stack.
    struct shared_loader_life_support_data {
        BIND11_TLS_KEY_INIT(loader_life_support_tls_key)
        shared_loader_life_support_data() {
            if (!BIND11_TLS_KEY_CREATE(loader_life_support_tls_key)) {
                BIND11_fail("local_internals: could not successfully initialize the "
                              "loader_life_support TLS key!");
            }
        }
        // We can't help but leak the TLS key, because Python never unloads extension modules.
    };

    local_internals() {
        auto &internals = get_internals();
        // Get or create the `loader_life_support_stack_key`.
        auto &ptr = internals.shared_data["_life_support"];
        if (!ptr) {
            ptr = new shared_loader_life_support_data;
        }
        loader_life_support_tls_key
            = static_cast<shared_loader_life_support_data *>(ptr)->loader_life_support_tls_key;
    }
#endif //  defined(WITH_THREAD) && BIND11_INTERNALS_VERSION == 4
};

/// Works like `get_internals`, but for things which are locally registered.
inline local_internals &get_local_internals() {
    static local_internals locals;
    return locals;
}

/// Constructs a std::string with the given arguments, stores it in `internals`, and returns its
/// `c_str()`.  Such strings objects have a long storage duration -- the internal strings are only
/// cleared when the program exits or after interpreter shutdown (when embedding), and so are
/// suitable for c-style strings needed by Python internals (such as TypeObject's tp_name).
template <typename... Args>
const char *c_str(Args &&...args) {
    auto &strings = get_internals().static_strings;
    strings.emplace_front(std::forward<Args>(args)...);
    return strings.front().c_str();
}

BIND11_NAMESPACE_END(detail)

/// Returns a named pointer that is shared among all extension modules (using the same
/// BIND11 version) running in the current interpreter. Names starting with underscores
/// are reserved for internal usage. Returns `nullptr` if no matching entry was found.
BIND11_NOINLINE void *get_shared_data(const std::string &name) {
    auto &internals = detail::get_internals();
    auto it = internals.shared_data.find(name);
    return it != internals.shared_data.end() ? it->second : nullptr;
}

/// Set the shared data that can be later recovered by `get_shared_data()`.
BIND11_NOINLINE void *set_shared_data(const std::string &name, void *data) {
    detail::get_internals().shared_data[name] = data;
    return data;
}

/// Returns a typed reference to a shared data entry (by using `get_shared_data()`) if
/// such entry exists. Otherwise, a new object of default-constructible type `T` is
/// added to the shared data under the given name and a reference to it is returned.
template <typename T>
T &get_or_create_shared_data(const std::string &name) {
    auto &internals = detail::get_internals();
    auto it = internals.shared_data.find(name);
    T *ptr = (T *) (it != internals.shared_data.end() ? it->second : nullptr);
    if (!ptr) {
        ptr = new T();
        internals.shared_data[name] = ptr;
    }
    return *ptr;
}

BIND11_NAMESPACE_END(BIND11_NAMESPACE)
