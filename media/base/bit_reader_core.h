//
// Created by wang rl on 2022/7/22.
//

#ifndef MULTIMEDIA_BIT_READER_CORE_H
#define MULTIMEDIA_BIT_READER_CORE_H

#include <cstdint>
#include <glog/logging.h>

namespace mm {
    class BitReaderCore {
    public:
        class ByteStreamProvider {
        public:
            ByteStreamProvider();

            virtual ~ByteStreamProvider();

            // Consume at most the following |max_n| bytes of the stream
            // and return the number n of bytes actually consumed.
            // Set |*array| to point to a memory buffer containing those n bytes.
            // Note: |*array| must be valid until the next call to GetBytes
            // but there is no guarantee it is valid after.
            virtual int GetBytes(int max_n, const uint8_t** array) = 0;
        };

        // Lifetime of |byte_stream_provider| must be longer than BitReaderCore.
        explicit BitReaderCore(ByteStreamProvider* byte_stream_provider);

        BitReaderCore(const BitReaderCore&) = delete;

        BitReaderCore& operator=(const BitReaderCore&) = delete;

        ~BitReaderCore();

        // Read one bit from the stream and return it as a boolean in |*flag|.
        bool ReadFlag(bool* flag);

        // Read one bit from the stream and return it as a boolean in |*out|.
        // Remark: we do not use the template version for reading a bool
        // since it generates some optimization warnings during compilation
        // on Windows platforms.
        bool ReadBits(int num_bits, bool* out) {
            DCHECK_EQ(num_bits, 1);
            return ReadFlag(out);
        }
    };
} // mm

#endif //MULTIMEDIA_BIT_READER_CORE_H
