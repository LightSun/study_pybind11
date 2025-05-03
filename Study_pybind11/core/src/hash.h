#pragma once
#include "core/src/c_common.h"

CPP_START

uint64 fasthash64(const void *buf, uint32 len, uint64 seed);

uint32 fasthash32(const void *buf, uint32 len, uint32 seed);

CPP_END
