//
// Created by wangrl2016 on 2022/7/21.
//

#include <gtest/gtest.h>
#include "media/filters/memory_protocol.h"

namespace mm {
    static const uint8_t kData[] = {0x01, 0x02, 0x03, 0x04};

    TEST(MemoryProtocolTest, ReadFromLargeBuffer) { // NOLINT
        MemoryProtocol protocol(kData, std::numeric_limits<int64_t>::max());

        uint8_t out[sizeof(kData)];
        EXPECT_EQ(4, protocol.Read(sizeof(out), out));
        EXPECT_EQ(0, memcmp(out, kData, sizeof(out)));
    }

    TEST(MemoryProtocolTest, ReadWithNegativeSize) {    // NOLINT
        MemoryProtocol protocol(kData, sizeof(kData));

        uint8_t out[sizeof(kData)];
        EXPECT_EQ(AVERROR(EIO), protocol.Read(-2, out));
    }

    TEST(MemoryPROtocolTest, ReadWithZeroSize) {    // NOLINT
        MemoryProtocol protocol(kData, sizeof(kData));

        uint8_t out;
        EXPECT_EQ(0, protocol.Read(0, &out));
    }

    TEST(MemoryProtocol, SetPosition) {
        MemoryProtocol protocol(kData, sizeof(kData));

        EXPECT_FALSE(protocol.SetPosition(-1));
        EXPECT_FALSE(protocol.SetPosition(sizeof(kData) + 1));

        uint8_t out;
        EXPECT_TRUE(protocol.SetPosition(sizeof(kData)));
        EXPECT_EQ(AVERROR_EOF, protocol.Read(1, &out));

        int i = sizeof(kData) / 2;
        EXPECT_TRUE(protocol.SetPosition(i));
        EXPECT_EQ(1, protocol.Read(1, &out));
        EXPECT_EQ(kData[i], out);
    }
}
