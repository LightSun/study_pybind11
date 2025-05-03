#pragma once

//_WIN32, _MSC_VER
#ifdef _WIN32
// For loadLibrary
// Needed so that the max/min definitions in windows.h do not conflict with std::max/min.
#define NOMINMAX
#include <windows.h>
#undef NOMINMAX
#else
#include <dlfcn.h>
#endif

#ifdef _MSC_VER
#define FN_NAME __FUNCTION__
#else
#define FN_NAME __func__
#endif

#include "Med_ai_acc/med_infer_common.h"

namespace h7 {
class DynamicLibrary
{
public:
    explicit DynamicLibrary(std::string const& name)
        : mLibName{name}
    {
#if defined(_WIN32)
        mHandle = LoadLibraryA(name.c_str());
#else // defined(_WIN32)
        int32_t flags{RTLD_LAZY};
#if ENABLE_ASAN
        // https://github.com/google/sanitizers/issues/89
        // asan doesn't handle module unloading correctly and there are no plans on doing
        // so. In order to get proper stack traces, don't delete the shared library on
        // close so that asan can resolve the symbols correctly.
        flags |= RTLD_NODELETE;
#endif // ENABLE_ASAN

        mHandle = dlopen(name.c_str(), flags);
#endif // defined(_WIN32)

        if (mHandle == nullptr)
        {
            std::string errorStr{};
#if !defined(_WIN32)
            errorStr = std::string{" due to "} + std::string{dlerror()};
#endif
            throw std::runtime_error("Unable to open library: " + name + errorStr);
        }
    }

    DynamicLibrary(DynamicLibrary const&) = delete;
    DynamicLibrary(DynamicLibrary const&&) = delete;

    //!
    //! Retrieve a function symbol from the loaded library.
    //! std::function<void*(void*, int32_t)> pCreateInferRuntimeInternal{};
    //! demo:  'pCreateInferRuntimeInternal = l->symbolAddress<void*(void*, int32_t)>("createInferRuntime_INTERNAL")';

    //! \return the loaded symbol on success
    //! \throw std::invalid_argument if loading the symbol failed.
    //!
    template <typename Signature>
    std::function<Signature> symbolAddress(char const* name)
    {
        if (mHandle == nullptr)
        {
            throw std::runtime_error("Handle to library is nullptr.");
        }
        void* ret;
#if defined(_WIN32)
        ret = (void*)(GetProcAddress(static_cast<HMODULE>(mHandle), name));
#else
        ret = dlsym(mHandle, name);
#endif
        if (ret == nullptr)
        {
            std::string const kERROR_MSG(mLibName + ": error loading symbol: " + std::string(name));
            throw std::invalid_argument(kERROR_MSG);
        }
        return reinterpret_cast<Signature*>(ret);
    }

    template <typename Signature>
    Signature* symbolAddress2(char const* name)
    {
        if (mHandle == nullptr)
        {
            throw std::runtime_error("Handle to library is nullptr.");
        }
        void* ret;
#if defined(_WIN32)
        ret = (void*)(GetProcAddress(static_cast<HMODULE>(mHandle), name));
#else
        ret = dlsym(mHandle, name);
#endif
        if (ret == nullptr)
        {
            std::string const kERROR_MSG(mLibName + ": error loading symbol: " + std::string(name));
            throw std::invalid_argument(kERROR_MSG);
        }
        return reinterpret_cast<Signature*>(ret);
    }

    ~DynamicLibrary()
    {
        try
        {
#if defined(_WIN32)
            MED_ASSERT(static_cast<bool>(FreeLibrary(static_cast<HMODULE>(mHandle))));
#else
            MED_ASSERT(dlclose(mHandle) == 0);
#endif
        }
        catch (...)
        {
            std::cerr << "Unable to close library: " << mLibName << std::endl;
        }
    }

private:
    std::string mLibName{};
    void* mHandle{};
};

inline std::unique_ptr<DynamicLibrary> loadLibrary(std::string const& path)
{
    // make_unique not available until C++14 - we still need to support C++11 builds.
    return std::unique_ptr<DynamicLibrary>(new DynamicLibrary{path});
}

}
