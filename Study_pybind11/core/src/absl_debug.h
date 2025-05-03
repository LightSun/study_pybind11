#pragma once

#ifdef USE_ABSL
#include "absl/debugging/failure_signal_handler.h"
#include "absl/debugging/symbolize.h"
#else
//
#endif

namespace h7 {

static inline void applyABSL(const char* arg0){
#ifdef USE_ABSL
    absl::InitializeSymbolizer(arg0);

    absl::FailureSignalHandlerOptions options;
    options.symbolize_stacktrace = true;
    options.alarm_on_failure_secs = 1;
    absl::InstallFailureSignalHandler(options);
#endif
}

}
