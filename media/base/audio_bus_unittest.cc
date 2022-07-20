//
// Created by wangrl2016 on 2022/7/19.
//
#include <gtest/gtest.h>
#include "media/base/audio_bus.h"

namespace mm {
    static const int kChannels = 6;
    // Use a buffer size which is intentionally not a multiple of kChannelAlignment.
    static const int kFrameCount = AudioBus::kChannelAlignment * 32 - 1;
    static const int kSampleRate = 48000;

    class AudioBusTest : public testing::Test {
    public:
        AudioBusTest() = default;

        AudioBusTest(const AudioBusTest&) = delete;

        AudioBusTest& operator=(const AudioBusTest&) = delete;

        ~AudioBusTest() override {
            for (size_t i = 0;i  < data_.size(); i++)
                AlignedFree(data_[i]);
        }

        void VerifyChannelAndFrameCount(AudioBus* bus) {
            EXPECT_EQ(kChannels, bus->channels());
            EXPECT_EQ(kFrameCount, bus->frames());
        }

        void VerifyArrayIsFilledWithValue(const float data[], int size, float value) {
            for (int i = 0; i < size; i++)
                ASSERT_FLOAT_EQ(value, data[i]) << "i = " << i;
        }

    protected:
        std::vector<float*> data_;
    };

    // Verify base Create(...) method works as advertised.
    TEST_F(AudioBusTest, Create) {  // NOLINT
        std::unique_ptr<AudioBus> bus = AudioBus::Create(kChannels, kFrameCount);
        VerifyChannelAndFrameCount(bus.get());
    }
}
