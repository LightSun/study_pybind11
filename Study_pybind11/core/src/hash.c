

#include "hash.h"


// Compression function for Merkle-Damgard construction.
// This function is generated using the framework provided.
#ifdef _WIN32
static inline uint64 mix(uint64 h){
    (h) ^= (h) >> 23;
    (h) *= 0x2127599bf4325c37ULL;
    (h) ^= (h) >> 47;
    return h;
}
#else
#define mix(h) ({					\
            (h) ^= (h) >> 23;		\
            (h) *= 0x2127599bf4325c37ULL;	\
            (h) ^= (h) >> 47; })
#endif

//https://github.com/ztanml/fast-hash/blob/master/fasthash.c
inline uint64 fasthash64(const void *buf, uint32 len, uint64 seed){
    const uint64    m = 0x880355f21e6d1965ULL;
    const uint64 *pos = (const uint64 *)buf;
    const uint64 *end = pos + (len / 8);
    const unsigned char *pos2;
    uint64 h = seed ^ (len * m);
    uint64 v;

    while (pos != end) {
        v  = *pos++;
        h ^= mix(v);
        h *= m;
    }

    pos2 = (const unsigned char*)pos;
    v = 0;

    switch (len & 7) {
    case 7: v ^= (uint64)pos2[6] << 48;
    case 6: v ^= (uint64)pos2[5] << 40;
    case 5: v ^= (uint64)pos2[4] << 32;
    case 4: v ^= (uint64)pos2[3] << 24;
    case 3: v ^= (uint64)pos2[2] << 16;
    case 2: v ^= (uint64)pos2[1] << 8;
    case 1: v ^= (uint64)pos2[0];
        h ^= mix(v);
        h *= m;
    }

    return mix(h);
}

uint32 fasthash32(const void *buf, uint32 len, uint32 seed){
    // the following trick converts the 64-bit hashcode to Fermat
    // residue, which shall retain information from both the higher
    // and lower parts of hashcode.
    uint64 h = fasthash64(buf, len, seed);
    return h - (h >> 32);
}

