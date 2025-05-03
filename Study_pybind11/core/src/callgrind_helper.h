#pragma once

#include <string>

#ifdef USE_CALLVARIND
#include "callgrind.h"
#endif

namespace h7 {

static inline void callgrind_collect(){
#ifdef USE_CALLVARIND
CALLGRIND_TOGGLE_COLLECT;
#endif
}

static inline void callgrind_stat(const std::string& str){
#ifdef USE_CALLVARIND
if(str.empty()){
    CALLGRIND_DUMP_STATS;
}else{
    CALLGRIND_DUMP_STATS_AT(str.data());
}
#endif
}

static inline void callgrind_begin(){
    callgrind_collect();
}

static inline void callgrind_end(const std::string& str){
    callgrind_collect();
    callgrind_stat(str);
}

}
