//
// Created by wangrl2016 on 2022/7/21.
//

#include "media/filters/memory_protocol.h"

namespace mm {
    MemoryProtocol::MemoryProtocol(const uint8_t* data, int64_t size) :
            data_(data), size_(size >= 0 ? size : 0), position_(0) {}

    MemoryProtocol::~MemoryProtocol() = default;

    int MemoryProtocol::Read(int size, uint8_t* data) {
        // Not sure if this can happen, but it's unclear from the ffmpeg code, so guard
        // against it.
        if (size < 0)
            return AVERROR(EIO);
        if (!size)
            return 0;

        const int64_t available_bytes = size_ - position_;
        if (available_bytes <= 0)
            return AVERROR_EOF;

        if (size > available_bytes)
            size = int(available_bytes);

        if (size > 0) {
            memcpy(data, data_ + position_, size);
            position_ += size;
        }

        return size;
    }

    bool MemoryProtocol::GetPosition(int64_t* position_out) {
        if (!position_out)
            return false;

        *position_out = position_;
        return true;
    }

    bool MemoryProtocol::SetPosition(int64_t position) {
        if (position < 0 || position > size_)
            return false;
        position_ = position;
        return true;
    }

    bool MemoryProtocol::GetSize(int64_t* size_out) {
        if (!size_out)
            return false;

        *size_out = size_;
        return true;
    }
}
