//
// Created by wangrl2016 on 2022/7/22.
//

#include "media/base/bit_reader.h"

namespace mm {
    BitReader::BitReader(const uint8_t* data, int size) :
            initial_size_(size),
            data_(data),
            bytes_left_(size),
            bit_reader_core_(this) {
        DCHECK(data != nullptr);
        DCHECK_GE(size, 0);
    }

    BitReader::~BitReader() = default;

    bool BitReader::ReadString(int num_bits, std::string* str) {
        DCHECK_EQ(num_bits % 8, 0);
        DCHECK_GT(num_bits, 0);
        DCHECK(str);
        int num_bytes = num_bits / 8;
        str->resize(num_bytes);
        char* ptr = &str->front();
        while (num_bytes--) {
            if (!ReadBits(8, ptr++))
                return false;
        }
        return true;
    }

    int BitReader::GetBytes(int max_n, const uint8_t** out) {
        DCHECK_GE(max_n, 0);
        DCHECK(out);

        int n_bytes = max_n;
        if (n_bytes > bytes_left_)
            n_bytes = bytes_left_;

        *out = data_;
        data_ += n_bytes;
        bytes_left_ -= n_bytes;
        return n_bytes;
    }
} // mm
