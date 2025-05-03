#pragma once


#define BIND11_VERSION_MAJOR 1
#define BIND11_VERSION_MINOR 0
#define BIND11_VERSION_PATCH 0.dev1

// Similar to Python's convention: https://docs.python.org/3/c-api/apiabiversion.html
// Additional convention: 0xD = dev
#define BIND11_VERSION_HEX 0x020A00D1

#define BIND11_NAMESPACE_BEGIN(name) namespace name {
#define BIND11_NAMESPACE_END(name) }

#if !defined(BIND11_NAMESPACE)
#    ifdef __GNUG__
#        define BIND11_NAMESPACE BIND11 __attribute__((visibility("hidden")))
#    else
#        define BIND11_NAMESPACE BIND11
#    endif
#endif

#if !(defined(_MSC_VER) && __cplusplus == 199711L)
#    if __cplusplus >= 201402L
#        define BIND11_CPP14
#        if __cplusplus >= 201703L
#            define BIND11_CPP17
#            if __cplusplus >= 202002L
#                define BIND11_CPP20
#            endif
#        endif
#    endif
#elif defined(_MSC_VER) && __cplusplus == 199711L
// MSVC sets _MSVC_LANG rather than __cplusplus (supposedly until the standard is fully
// implemented). Unless you use the /Zc:__cplusplus flag on Visual Studio 2017 15.7 Preview 3
// or newer.
#    if _MSVC_LANG >= 201402L
#        define BIND11_CPP14
#        if _MSVC_LANG > 201402L
#            define BIND11_CPP17
#            if _MSVC_LANG >= 202002L
#                define BIND11_CPP20
#            endif
#        endif
#    endif
#endif

// Compiler version assertions
#if defined(__INTEL_COMPILER)
#    if __INTEL_COMPILER < 1800
#        error BIND11 requires Intel C++ compiler v18 or newer
#    elif __INTEL_COMPILER < 1900 && defined(BIND11_CPP14)
#        error BIND11 supports only C++11 with Intel C++ compiler v18. Use v19 or newer for C++14.
#    endif
/* The following pragma cannot be pop'ed:
   https://community.intel.com/t5/Intel-C-Compiler/Inline-and-no-inline-warning/td-p/1216764 */
#    pragma warning disable 2196 // warning #2196: routine is both "inline" and "noinline"
#elif defined(__clang__) && !defined(__apple_build_version__)
#    if __clang_major__ < 3 || (__clang_major__ == 3 && __clang_minor__ < 3)
#        error BIND11 requires clang 3.3 or newer
#    endif
#elif defined(__clang__)
// Apple changes clang version macros to its Xcode version; the first Xcode release based on
// (upstream) clang 3.3 was Xcode 5:
#    if __clang_major__ < 5
#        error BIND11 requires Xcode/clang 5.0 or newer
#    endif
#elif defined(__GNUG__)
#    if __GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 8)
#        error BIND11 requires gcc 4.8 or newer
#    endif
#elif defined(_MSC_VER)
#    if _MSC_VER < 1910
#        error BIND11 2.10+ requires MSVC 2017 or newer
#    endif
#endif

#if !defined(BIND11_EXPORT)
#    if defined(WIN32) || defined(_WIN32)
#        define BIND11_EXPORT __declspec(dllexport)
#    else
#        define BIND11_EXPORT __attribute__((visibility("default")))
#    endif
#endif

#if !defined(BIND11_EXPORT_EXCEPTION)
#    ifdef __MINGW32__
// workaround for:
// error: 'dllexport' implies default visibility, but xxx has already been declared with a
// different visibility
#        define BIND11_EXPORT_EXCEPTION
#    else
#        define BIND11_EXPORT_EXCEPTION BIND11_EXPORT
#    endif
#endif

// For CUDA, GCC7, GCC8:
// BIND11_NOINLINE_FORCED is incompatible with `-Wattributes -Werror`.
// When defining BIND11_NOINLINE_FORCED, it is best to also use `-Wno-attributes`.
// However, the measured shared-library size saving when using noinline are only
// 1.7% for CUDA, -0.2% for GCC7, and 0.0% for GCC8 (using -DCMAKE_BUILD_TYPE=MinSizeRel,
// the default under BIND11/tests).
#if !defined(BIND11_NOINLINE_FORCED)                                                            \
    && (defined(__CUDACC__) || (defined(__GNUC__) && (__GNUC__ == 7 || __GNUC__ == 8)))
#    define BIND11_NOINLINE_DISABLED
#endif

// The BIND11_NOINLINE macro is for function DEFINITIONS.
// In contrast, FORWARD DECLARATIONS should never use this macro:
// https://stackoverflow.com/questions/9317473/forward-declaration-of-inline-functions
#if defined(BIND11_NOINLINE_DISABLED) // Option for maximum portability and experimentation.
#    define BIND11_NOINLINE inline
#elif defined(_MSC_VER)
#    define BIND11_NOINLINE __declspec(noinline) inline
#else
#    define BIND11_NOINLINE __attribute__((noinline)) inline
#endif

#if defined(__MINGW32__)
// For unknown reasons all BIND11_DEPRECATED member trigger a warning when declared
// whether it is used or not
#    define BIND11_DEPRECATED(reason)
#elif defined(BIND11_CPP14)
#    define BIND11_DEPRECATED(reason) [[deprecated(reason)]]
#else
#    define BIND11_DEPRECATED(reason) __attribute__((deprecated(reason)))
#endif

#if defined(BIND11_CPP17)
#    define BIND11_MAYBE_UNUSED [[maybe_unused]]
#elif defined(_MSC_VER) && !defined(__clang__)
#    define BIND11_MAYBE_UNUSED
#else
#    define BIND11_MAYBE_UNUSED __attribute__((__unused__))
#endif

//-------------------------------
#ifdef BIND_TRACE_REFS
/* Define pointers to support a doubly-linked list of all live heap objects. */
#define _Object_HEAD_EXTRA            \
    struct _object *_ob_next;           \
    struct _object *_ob_prev;

#define _Object_EXTRA_INIT 0, 0,

#else
#  define _Object_HEAD_EXTRA
#  define _Object_EXTRA_INIT
#endif
