//
// Created by wangrl2016 on 2022/7/21.
//

#include <gtest/gtest.h>
#include "media/filters/memory_protocol.h"

namespace mm {
    static const uint8_t kData[] = {0x01, 0x02, 0x03, 0x04};

    TEST(MemoryProtocolTest, ReadFromLargeBuffer) {
        MemoryProtocol protocol(kData, std::numeric_limits<int64_t>::max());

        uint8_t out[sizeof(kData)];
        EXPECT_EQ(4, protocol.Read(sizeof(out), out));
        EXPECT_EQ(0, memcmp(out, kData, sizeof(out)));
    }
}
