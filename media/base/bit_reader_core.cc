//
// Created by wang rl on 2022/7/22.
//

#include "base/system/byte_order.h"
#include "media/base/bit_reader_core.h"

namespace mm {
    const int kRegWidthInBits = sizeof(uint64_t) * 8;

    BitReaderCore::BitReaderCore(ByteStreamProvider* byte_stream_provider) :
            byte_stream_provider_(byte_stream_provider),
            bits_read_(0),
            num_bits_(0),
            reg_(0),
            num_bits_next_(0),
            reg_next_(0) {}

    bool BitReaderCore::ReadFlag(bool* flag) {
        if (num_bits_ == 0 && !Refill(1))
            return false;

        *flag = (reg_ & (UINT64_C(1) << (kRegWidthInBits - 1))) != 0;
        reg_ <<= 1;
        num_bits_--;
        bits_read_++;
        return true;
    }

    bool BitReaderCore::SkipBits(int num_bits) {
        DCHECK_GE(num_bits, 0);

        const int remaining_bits = num_bits + num_bits_next_;
        if (remaining_bits >= num_bits)
            return SkipBitsSmall(num_bits);

        // Skip first the remaining available bits.
        num_bits -= remaining_bits;
        bits_read_ += remaining_bits;
        num_bits_ = 0;
        reg_ = 0;
        num_bits_next_ = 0;
        reg_next_ = 0;

        // Next, skip an integer number of bytes.
        const int num_bytes = num_bits / 8;
        if (num_bytes > 0) {
            const uint8_t* byte_stream_window;
            const int window_size =
                    byte_stream_provider_->GetBytes(num_bytes, &byte_stream_window);
            DCHECK_GE(window_size, 0);
            DCHECK_LE(window_size, num_bytes);
            if (window_size < num_bytes) {
                // Note that some bytes were consumed.
                bits_read_ += 8 * window_size;
                return false;
            }
            num_bits -= 8 * num_bytes;
            bits_read_ += 8 * num_bytes;
        }

        // Skip the remaining bits.
        return SkipBitsSmall(num_bits);
    }

    int BitReaderCore::bits_read() const {
        return bits_read_;
    }

    bool BitReaderCore::SkipBitsSmall(int num_bits) {
        DCHECK_GE(num_bits, 0);
        uint64_t dummy;

        while (num_bits >= kRegWidthInBits) {
            if (!ReadBitsInternal(kRegWidthInBits, &dummy))
                return false;
            num_bits -= kRegWidthInBits;
        }
        return ReadBitsInternal(num_bits, &dummy);
    }

    bool BitReaderCore::ReadBitsInternal(int num_bits, uint64_t* out) {
        DCHECK_GE(num_bits, 0);

        if (num_bits == 0) {
            *out = 0;
            return true;
        }

        if (num_bits > num_bits_ && !Refill(num_bits)) {
            // Any subsequent ReadBits should fail:
            // empty the current bit register for that purpose.
            num_bits_ = 0;
            reg_ = 0;
            *out = 0;
            return false;
        }

        bits_read_ += num_bits;

        if (num_bits == kRegWidthInBits) {
            // Special case needed since for example for a 64-bit integer "a"
            // "a << 64" is not defined by the C/C++ standard.
            *out = reg_;
            reg_ = 0;
            num_bits_ = 0;
            return true;
        }

        *out = reg_ >> (kRegWidthInBits - num_bits);
        reg_ <<= num_bits;
        num_bits_ -= num_bits;
        return true;
    }

    bool BitReaderCore::Refill(int min_num_bits) {
        DCHECK_LE(min_num_bits, kRegWidthInBits);

        // Transfer from the next to the current register.
        RefillCurrentRegister();

        if (min_num_bits <= num_bits_)
            return true;
        DCHECK_EQ(num_bits_next_, 0);
        DCHECK_EQ(reg_next_, 0);

        // Max number of bytes to refill.
        int max_num_bytes = sizeof(reg_next_);

        // Refill.
        const uint8_t* byte_stream_window;
        int window_size =
                byte_stream_provider_->GetBytes(max_num_bytes, &byte_stream_window);
        DCHECK_GE(window_size, 0);
        DCHECK_LE(window_size, max_num_bytes);
        if (window_size == 0)
            return false;

        reg_next_ = 0;
        memcpy(&reg_next_, byte_stream_window, window_size);
        reg_next_ = NetToHost64(reg_next_);
        num_bits_next_ = window_size * 8;

        // Transfer from the next to the current register.
        RefillCurrentRegister();

        return (num_bits_ >= min_num_bits);
    }

    void BitReaderCore::RefillCurrentRegister() {
        // No refill possible if the destination register is full
        // or the source register is empty.
        if (num_bits_ == kRegWidthInBits || num_bits_next_ == 0)
            return;

        reg_ |= (reg_next_ >> num_bits_);

        // need bits from next register
        int free_num_bits = kRegWidthInBits - num_bits_;
        if (free_num_bits >= num_bits_next_) {
            num_bits_ += num_bits_next_;
            reg_next_ = 0;
            num_bits_next_ = 0;
            return;
        }

        num_bits_ += free_num_bits;
        reg_next_ <<= free_num_bits;
        num_bits_next_ -= free_num_bits;
    }
} // mm
