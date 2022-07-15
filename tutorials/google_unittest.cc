//
// Created by wangrl2016 on 2022/7/15.
//

#include <gtest/gtest.h>

TEST(HelloTest, BasicAssertions) {  // NOLINT
    // Expect two strings not to b equal.
    EXPECT_STRNE("hello", "world");
    // Expect equality.
    EXPECT_EQ(7 * 6, 42);
}
