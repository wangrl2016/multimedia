//
// Created by wangrl2016 on 2022/7/23.
//

#ifndef MULTIMEDIA_BYTE_ORDER_H
#define MULTIMEDIA_BYTE_ORDER_H

#include <cstdint>

namespace mm {
    inline uint16_t ByteSwap(uint16_t x) {
#if defined(_MSC_VER)
        return _byteswap_ushort(x);
#else
        return __builtin_bswap16(x);
#endif
    }

    inline uint32_t ByteSwap(uint32_t x) {
#if defined(_MSC_VER)
        return _byteswap_ulong(x);
#else
        return __builtin_bswap32(x);
#endif
    }

    inline uint64_t ByteSwap(uint64_t x) {
#if defined(_MSC_VER)
        return _byteswap_uint64(x);
#else
        return __builtin_bswap64(x);
#endif
    }

    inline uint64_t NetToHost64(uint64_t x) {
        // little endian
        return ByteSwap(x);
    }
}

#endif //MULTIMEDIA_BYTE_ORDER_H
