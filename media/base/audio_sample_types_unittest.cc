//
// Created by wangrl2016 on 2022/7/18.
//

#include <gtest/gtest.h>
#include "media/base/audio_sample_types.h"

namespace mm {
    template<typename TestConfig>
    class SampleTypeTraitsTest : public testing::Test {
    };

    TYPED_TEST_SUITE_P(SampleTypeTraitsTest);   // NOLINT

    struct UnsignedInt8ToFloat32TestConfig {
        using SourceTraits = UnsignedInt8SampleTypeTraits;
        using TargetTraits = Float32SampleTypeTraits;

        static const char* config_name() {
            return "UnsignedInt8ToFloat32TestConfig";
        };

        static TargetTraits::ValueType PerformConversion(
                SourceTraits::ValueType source_value) {
            return SourceTraits::ToFloat(source_value);
        }
    };

    TYPED_TEST_P(SampleTypeTraitsTest, ConvertExampleValues) {  // NOLINT
        using Config = TypeParam;
        using SourceTraits = typename Config::SourceTraits;
        using TargetTraits = typename Config::TargetTraits;
        using SourceType = typename SourceTraits::ValueType;
        using TargetType = typename TargetTraits::ValueType;
        TargetType target_zero_point = TargetTraits::kZeroPointValue;
        SCOPED_TRACE(Config::config_name());
        {
            SCOPED_TRACE("Convert zero-point value");
            ASSERT_EQ(target_zero_point,
                      Config::PerformConversion(SourceTraits::kZeroPointValue));
        }
    }

    REGISTER_TYPED_TEST_SUITE_P(SampleTypeTraitsTest, ConvertExampleValues);    // NOLINT

    typedef ::testing::Types<UnsignedInt8ToFloat32TestConfig>
            TestConfigs;
    INSTANTIATE_TYPED_TEST_SUITE_P(CommonTypes, SampleTypeTraitsTest, TestConfigs); // NOLINT
}
