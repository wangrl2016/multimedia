//
// Created by wangrl2016 on 2022/7/19.
//

#ifndef MULTIMEDIA_ALIGNED_MEMORY_H
#define MULTIMEDIA_ALIGNED_MEMORY_H

#include <bit>

#if defined(_MSC_VER)
#include <malloc.h>
#else
#include <cstdlib>
#endif

#include <glog/logging.h>

// A runtime sized aligned allocation can be created:
//
// float* my_array = static_cast<float*>(AlignedAlloc(size, alignment));
//
// // ... later, to release the memory:
// AlignedFree(my_array);
//
// Or using unique_ptr:
//
// std::unique_ptr<float, AlignedFreeDeleter> my_array(
//     static_cast<float*>(AlignedAlloc(size, alignment)));

namespace mm {
    void* AlignedAlloc(size_t size, size_t alignment);

    inline void AlignedFree(void* ptr) {
#if defined(_MSC_VER)
        _aligned_free(ptr);
#else
        free(ptr);
#endif
    }

    // Deleter for use with unique_ptr. e.g. use as
    // std::unique_ptr<Foo, base::AlignedFreeDeleter> foo;
    struct AlignedFreeDeleter {
        inline void operator()(void* ptr) const {
            AlignedFree(ptr);
        }
    };

    inline bool IsAligned(uintptr_t val, size_t alignment) {
        DCHECK(std::has_single_bit(alignment)) << alignment << " is not a power of 2";
        return (val & (alignment - 1)) == 0;
    }

    inline bool IsAligned(const void* val, size_t alignment) {
        return IsAligned(reinterpret_cast<uintptr_t>(val), alignment);
    }
}

#endif //MULTIMEDIA_ALIGNED_MEMORY_H
