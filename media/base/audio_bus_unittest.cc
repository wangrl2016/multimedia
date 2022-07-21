//
// Created by wangrl2016 on 2022/7/19.
//

#pragma clang diagnostic push
#pragma ide diagnostic ignored "bugprone-suspicious-memory-comparison"

#include <cmath>
#include <gtest/gtest.h>
#include "media/base/audio_bus.h"
#include "media/base/audio_sample_types.h"

namespace mm {
    static const int kChannels = 6;
    // Use a buffer size which is intentionally not a multiple of kChannelAlignment.
    static const int kFrameCount = AudioBus::kChannelAlignment * 32 - 1;

    class AudioBusTest : public testing::Test {
    public:
        AudioBusTest() = default;

        AudioBusTest(const AudioBusTest&) = delete;

        AudioBusTest& operator=(const AudioBusTest&) = delete;

        ~AudioBusTest() override {
            for (auto& i: data_)
                AlignedFree(i);
        }

        static void VerifyChannelAndFrameCount(AudioBus* bus) {
            EXPECT_EQ(kChannels, bus->channels());
            EXPECT_EQ(kFrameCount, bus->frames());
        }

        static void VerifyArrayIsFilledWithValue(const float data[], int size, float value) {
            for (int i = 0; i < size; i++)
                ASSERT_FLOAT_EQ(value, data[i]) << "i = " << i;
        }

        // Verify values for each channel in |result| are within |epsilon| of
        // |expected|.  If |epsilon| exactly equals 0, uses FLOAT_EQ macro.
        static void VerifyAreEqualWithEpsilon(const AudioBus* result,
                                              const AudioBus* expected,
                                              float epsilon) {
            ASSERT_EQ(expected->channels(), result->channels());
            ASSERT_EQ(expected->frames(), result->frames());

            for (int ch = 0; ch < result->channels(); ++ch) {
                for (int i = 0; i < result->frames(); ++i) {
                    if (epsilon == 0) {
                        ASSERT_FLOAT_EQ(expected->channel(ch)[i], result->channel(ch)[i]);
                    } else {
                        ASSERT_NEAR(expected->channel(ch)[i], result->channel(ch)[i],
                                    epsilon);
                    }
                }
            }
        }

        // Verify values for each channel in |result| against |expected|.
        static void VerifyAreEqual(const AudioBus* result, const AudioBus* expected) {
            VerifyAreEqualWithEpsilon(result, expected, 0);
        }

        // Read and write to the full extent of the allocated channel data.  Also test
        // the Zero() method and verify it does as advertised.  Also test data if data
        // is 16-byte aligned as advertised (see kChannelAlignment in audio_bus.h).
        static void VerifyReadWriteAndAlignment(AudioBus* bus) {
            for (int i = 0; i < bus->channels(); ++i) {
                // Verify that the address returned by channel(i) is a multiple of
                // AudioBus::kChannelAlignment.
                ASSERT_EQ(0U, reinterpret_cast<uintptr_t>(
                                      bus->channel(i)) & (AudioBus::kChannelAlignment - 1));

                // Write into the channel buffer.
                std::fill(bus->channel(i), bus->channel(i) + bus->frames(), i);
            }

            for (int i = 0; i < bus->channels(); ++i)
                VerifyArrayIsFilledWithValue(bus->channel(i), bus->frames(), float(i));

            bus->Zero();
            for (int i = 0; i < bus->channels(); ++i)
                VerifyArrayIsFilledWithValue(bus->channel(i), bus->frames(), 0);
        }

        // Verify copying to and from |bus1| and |bus2|.
        static void CopyTest(AudioBus* bus1, AudioBus* bus2) {
            // Fill |bus1| with dummy data.
            for (int i = 0; i < bus1->channels(); ++i)
                std::fill(bus1->channel(i), bus1->channel(i) + bus1->frames(), i);

            // Verify copy from |bus1| to |bus2|.
            bus2->Zero();
            bus1->CopyTo(bus2);
            VerifyAreEqual(bus1, bus2);

            // Verify copy from |bus2| to |bus1|.
            bus1->Zero();
            bus2->CopyTo(bus1);
            VerifyAreEqual(bus2, bus1);
        }

    protected:
        std::vector<float*> data_;
    };

    // Verify base Create(...) method works as advertised.
    TEST_F(AudioBusTest, Create) {  // NOLINT
        std::unique_ptr<AudioBus> bus = AudioBus::Create(kChannels, kFrameCount);
        VerifyChannelAndFrameCount(bus.get());
        VerifyReadWriteAndAlignment(bus.get());
    }

    // Verify an AudioBus created via wrapping a vector works as advertised.
    TEST_F(AudioBusTest, WrapVector) {  // NOLINT
        data_.reserve(kChannels);
        for (int i = 0; i < kChannels; ++i) {
            data_.push_back(static_cast<float*>(AlignedAlloc(
                    sizeof(*data_[i]) * kFrameCount, AudioBus::kChannelAlignment)));
        }

        std::unique_ptr<AudioBus> bus = AudioBus::WrapVector(kFrameCount, data_);
        VerifyChannelAndFrameCount(bus.get());
        VerifyReadWriteAndAlignment(bus.get());
    }

    // Verify an AudioBus created via wrapping a memory block works as advertised.
    TEST_F(AudioBusTest, WrapMemory) {  // NOLINT
        int data_size = AudioBus::CalculateMemorySize(kChannels, kFrameCount);
        std::unique_ptr<float, AlignedFreeDeleter> data(
                static_cast<float*>(
                        AlignedAlloc(data_size, AudioBus::kChannelAlignment)));

        // Fill the memory with a test value we can check for after wrapping.
        static const float kTestValue = 3;
        std::fill(
                data.get(), data.get() + data_size / sizeof(*data), kTestValue);

        std::unique_ptr<AudioBus> bus = AudioBus::WrapMemory(
                kChannels, kFrameCount, data.get());
        // Verify the test value we filled prior to wrapping.
        for (int i = 0; i < bus->channels(); ++i)
            VerifyArrayIsFilledWithValue(bus->channel(i), bus->frames(), kTestValue);
        VerifyChannelAndFrameCount(bus.get());
        VerifyReadWriteAndAlignment(bus.get());

        // Verify the channel vectors lie within the provided memory block.
        EXPECT_GE(bus->channel(0), data.get());
        EXPECT_LT(bus->channel(bus->channels() - 1) + bus->frames(),
                  data.get() + data_size / sizeof(*data.get()));
    }

    // Simulate a shared memory transfer and verify results.
    TEST_F(AudioBusTest, CopyTo) {  // NOLINT
        std::unique_ptr<AudioBus> bus1 = AudioBus::Create(kChannels, kFrameCount);
        std::unique_ptr<AudioBus> bus2 = AudioBus::Create(kChannels, kFrameCount);

        {
            SCOPED_TRACE("Created");
            CopyTest(bus1.get(), bus2.get());
        }
        {
            SCOPED_TRACE("Wrapped Vector");
            // Try a copy to an AudioBus wrapping a vector.
            data_.reserve(kChannels);
            for (int i = 0; i < kChannels; ++i) {
                data_.push_back(static_cast<float*>(AlignedAlloc(
                        sizeof(*data_[i]) * kFrameCount, AudioBus::kChannelAlignment)));
            }

            bus2 = AudioBus::WrapVector(kFrameCount, data_);
            CopyTest(bus1.get(), bus2.get());
        }
        {
            SCOPED_TRACE("Wrapped Memory");
            // Try a copy to an AudioBus wrapping a memory block.
            std::unique_ptr<float, AlignedFreeDeleter> data(
                    static_cast<float*>(
                            AlignedAlloc(
                                    AudioBus::CalculateMemorySize(kChannels,
                                                                  kFrameCount),
                                    AudioBus::kChannelAlignment)));

            bus2 = AudioBus::WrapMemory(kChannels, kFrameCount, data.get());
            CopyTest(bus1.get(), bus2.get());
        }
    }

    // Verify Zero() and ZeroFrames(...) utility methods work as advertised.
    TEST_F(AudioBusTest, Zero) {    // NOLINT
        std::unique_ptr<AudioBus> bus = AudioBus::Create(kChannels, kFrameCount);

        // Fill the bus with dummy data.
        for (int i = 0; i < bus->channels(); ++i)
            std::fill(bus->channel(i), bus->channel(i) + bus->frames(), i + 1);
        EXPECT_FALSE(bus->AreFramesZero());

        // Zero first half the frames of each channel.
        bus->ZeroFrames(kFrameCount / 2);
        for (int i = 0; i < bus->channels(); ++i) {
            SCOPED_TRACE("First Half Zero");
            VerifyArrayIsFilledWithValue(bus->channel(i), kFrameCount / 2, 0);
            VerifyArrayIsFilledWithValue(bus->channel(i) + kFrameCount / 2,
                                         kFrameCount - kFrameCount / 2, float(i + 1));
        }
        EXPECT_FALSE(bus->AreFramesZero());

        // Fill the bus with dummy data.
        for (int i = 0; i < bus->channels(); ++i)
            std::fill(bus->channel(i), bus->channel(i) + bus->frames(), i + 1);

        // Zero the last half of the frames.
        bus->ZeroFramesPartial(kFrameCount / 2, kFrameCount - kFrameCount / 2);
        for (int i = 0; i < bus->channels(); ++i) {
            SCOPED_TRACE("Last Half Zero");
            VerifyArrayIsFilledWithValue(bus->channel(i) + kFrameCount / 2,
                                         kFrameCount - kFrameCount / 2, 0);
            VerifyArrayIsFilledWithValue(bus->channel(i), kFrameCount / 2, float(i + 1));
        }
        EXPECT_FALSE(bus->AreFramesZero());

        // Fill the bus with dummy data.
        for (int i = 0; i < bus->channels(); ++i)
            std::fill(bus->channel(i), bus->channel(i) + bus->frames(), i + 1);

        // Zero all the frames of each channel.
        bus->Zero();
        for (int i = 0; i < bus->channels(); ++i) {
            SCOPED_TRACE("All Zero");
            VerifyArrayIsFilledWithValue(bus->channel(i), bus->frames(), 0);
        }
        EXPECT_TRUE(bus->AreFramesZero());
    }

    // Each test vector represents two channels of data in the following arbitrary
    // layout: <min, zero, max, min, max / 2, min / 2, zero, max, zero, zero>.
    static const int kTestVectorSize = 10;
    static const uint8_t kTestVectorUint8[kTestVectorSize] = {
            0, -INT8_MIN, UINT8_MAX,
            0, INT8_MAX / 2 + 128, INT8_MIN / 2 + 128,
            -INT8_MIN, UINT8_MAX, -INT8_MIN,
            -INT8_MIN};
    static const int16_t kTestVectorInt16[kTestVectorSize] = {
            INT16_MIN, 0, INT16_MAX, INT16_MIN, INT16_MAX / 2,
            INT16_MIN / 2, 0, INT16_MAX, 0, 0};
    static const int32_t kTestVectorInt32[kTestVectorSize] = {
            INT32_MIN, 0, INT32_MAX, INT32_MIN, INT32_MAX / 2,
            INT32_MIN / 2, 0, INT32_MAX, 0, 0};
    static const float kTestVectorFloat32[kTestVectorSize] = {
            -1.0f, 0.0f, 1.0f, -1.0f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f};

    // This is based on kTestVectorFloat32, but has some values outside of
    // sanity.
    static const float kTestVectorFloat32Invalid[kTestVectorSize] = {
            -5.0f,
            0.0f,
            5.0f,
            -1.0f,
            0.5f,
            -0.5f,
            0.0f,
            std::numeric_limits<float>::infinity(),
            std::numeric_limits<float>::signaling_NaN(),
            std::numeric_limits<float>::quiet_NaN()};

    static const float kTestVectorFloat32Sanitized[kTestVectorSize] = {
            -1.0f, 0.0f, 1.0f, -1.0f, 0.5f, -0.5f, 0.0f, 1.0f, -1.0f, -1.0f};

// Expected results.
    static const int kTestVectorFrameCount = kTestVectorSize / 2;
    static const float kTestVectorResult[][kTestVectorFrameCount] = {
            {-1.0f, 1.0f,  0.5f,  0.0f, 0.0f},
            {0.0f,  -1.0f, -0.5f, 1.0f, 0.0f}};
    static const int kTestVectorChannelCount = std::size(kTestVectorResult);

    // Verify FromInterleaved() de-interleaves audio in supported formats correctly.
    TEST_F(AudioBusTest, FromInterleaved) { // NOLINT
        std::unique_ptr<AudioBus> bus =
                AudioBus::Create(kTestVectorChannelCount, kTestVectorFrameCount);
        std::unique_ptr<AudioBus> expected =
                AudioBus::Create(kTestVectorChannelCount, kTestVectorFrameCount);
        for (int ch = 0; ch < kTestVectorChannelCount; ++ch) {
            memcpy(expected->channel(ch), kTestVectorResult[ch],
                   kTestVectorFrameCount * sizeof(*expected->channel(ch)));
        }

        {
            SCOPED_TRACE("UnsignedInt8SampleTypeTraits");
            bus->Zero();
            bus->FromInterleaved<UnsignedInt8SampleTypeTraits>(kTestVectorUint8,
                                                               kTestVectorFrameCount);
            // Biased uint8_t calculations have poor precision, so the epsilon here is
            // slightly more permissive than int16_t and int32_t calculations.
            VerifyAreEqualWithEpsilon(bus.get(), expected.get(),
                                      1.0f / (std::numeric_limits<uint8_t>::max() - 1));
        }
        {
            SCOPED_TRACE("SignedInt16SampleTypeTraits");
            bus->Zero();
            bus->FromInterleaved<SignedInt16SampleTypeTraits>(kTestVectorInt16,
                                                              kTestVectorFrameCount);
            VerifyAreEqualWithEpsilon(
                    bus.get(), expected.get(),
                    1.0f /
                    (static_cast<float>(std::numeric_limits<uint16_t>::max()) + 1.0f));
        }
        {
            SCOPED_TRACE("SignedInt32SampleTypeTraits");
            bus->Zero();
            bus->FromInterleaved<SignedInt32SampleTypeTraits>(kTestVectorInt32,
                                                              kTestVectorFrameCount);
            VerifyAreEqualWithEpsilon(
                    bus.get(), expected.get(),
                    1.0f / static_cast<float>(std::numeric_limits<uint32_t>::max()));
        }
        {
            SCOPED_TRACE("Float32SampleTypeTraits");
            bus->Zero();
            bus->FromInterleaved<Float32SampleTypeTraits>(kTestVectorFloat32,
                                                          kTestVectorFrameCount);
            VerifyAreEqual(bus.get(), expected.get());
        }
    }

    // Verify FromInterleavedPartial() de-interleaves audio correctly.
    TEST_F(AudioBusTest, FromInterleavedPartial) {  // NOLINT
        // Only de-interleave the middle two frames in each channel.
        static const int kPartialStart = 1;
        static const int kPartialFrames = 2;
        ASSERT_LE(kPartialStart + kPartialFrames, kTestVectorFrameCount);

        std::unique_ptr<AudioBus> bus =
                AudioBus::Create(kTestVectorChannelCount, kTestVectorFrameCount);
        std::unique_ptr<AudioBus> expected =
                AudioBus::Create(kTestVectorChannelCount, kTestVectorFrameCount);
        expected->Zero();
        for (int ch = 0; ch < kTestVectorChannelCount; ++ch) {
            memcpy(expected->channel(ch) + kPartialStart,
                   kTestVectorResult[ch] + kPartialStart,
                   kPartialFrames * sizeof(*expected->channel(ch)));
        }

        {
            SCOPED_TRACE("SignedInt32SampleTypeTraits");
            bus->Zero();
            bus->FromInterleavedPartial<SignedInt32SampleTypeTraits>(
                    kTestVectorInt32 + kPartialStart * bus->channels(), kPartialStart,
                    kPartialFrames);
            VerifyAreEqual(bus.get(), expected.get());
        }
    }

    // Verify ToInterleaved() interleaves audio in supported formats correctly.
    TEST_F(AudioBusTest, ToInterleaved) {   // NOLINT
        std::unique_ptr<AudioBus> bus =
                AudioBus::Create(kTestVectorChannelCount, kTestVectorFrameCount);
        // Fill the bus with our test vector.
        for (int ch = 0; ch < bus->channels(); ++ch) {
            memcpy(bus->channel(ch), kTestVectorResult[ch],
                   kTestVectorFrameCount * sizeof(*bus->channel(ch)));
        }

        {
            SCOPED_TRACE("UnsignedInt8SampleTypeTraits");
            uint8_t test_array[std::size(kTestVectorUint8)];
            bus->ToInterleaved<UnsignedInt8SampleTypeTraits>(bus->frames(), test_array);
            ASSERT_EQ(0,
                      memcmp(test_array, kTestVectorUint8, sizeof(kTestVectorUint8)));
        }
        {
            SCOPED_TRACE("SignedInt16SampleTypeTraits");
            int16_t test_array[std::size(kTestVectorInt16)];
            bus->ToInterleaved<SignedInt16SampleTypeTraits>(bus->frames(), test_array);
            ASSERT_EQ(0,
                      memcmp(test_array, kTestVectorInt16, sizeof(kTestVectorInt16)));
        }
        {
            SCOPED_TRACE("SignedInt32SampleTypeTraits");
            int32_t test_array[std::size(kTestVectorInt32)];
            bus->ToInterleaved<SignedInt32SampleTypeTraits>(bus->frames(), test_array);

            // Some compilers get better precision than others on the half-max test, so
            // let the test pass with an off by one check on the half-max.
            int32_t alternative_acceptable_result[std::size(kTestVectorInt32)];
            memcpy(alternative_acceptable_result, kTestVectorInt32,
                   sizeof(kTestVectorInt32));
            ASSERT_EQ(alternative_acceptable_result[4],
                      std::numeric_limits<int32_t>::max() / 2);
            alternative_acceptable_result[4]++;

            ASSERT_TRUE(
                    memcmp(test_array, kTestVectorInt32, sizeof(kTestVectorInt32)) == 0 ||
                    memcmp(test_array, alternative_acceptable_result,
                           sizeof(alternative_acceptable_result)) == 0);
        }
        {
            SCOPED_TRACE("Float32SampleTypeTraits");
            float test_array[std::size(kTestVectorFloat32)];
            bus->ToInterleaved<Float32SampleTypeTraits>(bus->frames(), test_array);
            ASSERT_TRUE(0 == memcmp(test_array, kTestVectorFloat32, sizeof(kTestVectorFloat32)));
        }
    }

    TEST_F(AudioBusTest, ToInterleavedSanitized) {  // NOLINT
        std::unique_ptr<AudioBus> bus =
                AudioBus::Create(kTestVectorChannelCount, kTestVectorFrameCount);
        bus->FromInterleaved<Float32SampleTypeTraits>(kTestVectorFloat32Invalid,
                                                      bus->frames());
        // Verify FromInterleaved applied no sanity.
        ASSERT_EQ(bus->channel(0)[0], kTestVectorFloat32Invalid[0]);
        float test_array[std::size(kTestVectorFloat32Sanitized)];
        bus->ToInterleaved<Float32SampleTypeTraits>(bus->frames(), test_array);
        for (size_t i = 0; i < std::size(kTestVectorFloat32Sanitized); ++i)
            ASSERT_EQ(kTestVectorFloat32Sanitized[i], test_array[i]);

        // Verify that Float32SampleTypeTraitsNoClip applied no sanity.
        bus->ToInterleaved<Float32SampleTypeTraitsNoClip>(bus->frames(), test_array);
        for (int i = 0; i < kTestVectorSize; ++i) {
            if (std::isnan(test_array[i]))
                EXPECT_TRUE(std::isnan(kTestVectorFloat32Invalid[i]));
            else
                EXPECT_FLOAT_EQ(test_array[i], kTestVectorFloat32Invalid[i]);
        }
    }

    TEST_F(AudioBusTest, CopyAndClipTo) {   // NOLINT
        auto bus = AudioBus::Create(
                kTestVectorChannelCount, kTestVectorFrameCount);
        bus->FromInterleaved<Float32SampleTypeTraits>(kTestVectorFloat32Invalid,
                                                      bus->frames());
        auto expected =
                AudioBus::Create(kTestVectorChannelCount, kTestVectorFrameCount);
        expected->FromInterleaved<Float32SampleTypeTraits>(
                kTestVectorFloat32Sanitized, bus->frames());

        // Verify FromInterleaved applied no sanity.
        ASSERT_EQ(bus->channel(0)[0], kTestVectorFloat32Invalid[0]);

        std::unique_ptr<AudioBus> copy_to_bus =
                AudioBus::Create(kTestVectorChannelCount, kTestVectorFrameCount);
        bus->CopyAndClipTo(copy_to_bus.get());

        for (int ch = 0; ch < expected->channels(); ++ch) {
            for (int i = 0; i < expected->frames(); ++i)
                ASSERT_EQ(copy_to_bus->channel(ch)[i], expected->channel(ch)[i]);
        }

        ASSERT_EQ(expected->channels(), copy_to_bus->channels());
        ASSERT_EQ(expected->frames(), copy_to_bus->frames());
    }

    // Verify ToInterleavedPartial() interleaves audio correctly.
    TEST_F(AudioBusTest, ToInterleavedPartial) {    // NOLINT
        // Only interleave the middle two frames in each channel.
        static const int kPartialStart = 1;
        static const int kPartialFrames = 2;
        ASSERT_LE(kPartialStart + kPartialFrames, kTestVectorFrameCount);

        std::unique_ptr<AudioBus> expected =
                AudioBus::Create(kTestVectorChannelCount, kTestVectorFrameCount);
        for (int ch = 0; ch < kTestVectorChannelCount; ++ch) {
            memcpy(expected->channel(ch), kTestVectorResult[ch],
                   kTestVectorFrameCount * sizeof(*expected->channel(ch)));
        }

        {
            SCOPED_TRACE("Float32SampleTypeTraits");
            float test_array[std::size(kTestVectorFloat32)];
            expected->ToInterleavedPartial<Float32SampleTypeTraits>(
                    kPartialStart, kPartialFrames, test_array);
            ASSERT_TRUE(0 == memcmp(test_array, kTestVectorFloat32 +
                                                kPartialStart * kTestVectorChannelCount,
                                    kPartialFrames * sizeof(*kTestVectorFloat32) *
                                    kTestVectorChannelCount));
        }
    }

    struct ZeroingOutTestData {
        static constexpr int kChannelCount = 2;
        static constexpr int kFrameCount = 10;
        static constexpr int kInterleavedFrameCount = 3;

        std::unique_ptr<AudioBus> bus_under_test;
        std::vector<float> interleaved_dummy_frames;

        ZeroingOutTestData() {
            // Create a bus and fill each channel with a test pattern of form
            // [1.0, 2.0, 3.0, ...]
            bus_under_test = AudioBus::Create(kChannelCount, kFrameCount);
            for (int ch = 0; ch < kChannelCount; ++ch) {
                auto* sample_array_for_current_channel = bus_under_test->channel(ch);
                for (int frame_index = 0; frame_index < kFrameCount; frame_index++) {
                    sample_array_for_current_channel[frame_index] =
                            static_cast<float>(frame_index + 1);
                }
            }

            // Create a vector containing dummy interleaved samples.
            static const float kDummySampleValue = 0.123f;
            interleaved_dummy_frames.resize(kChannelCount * kInterleavedFrameCount);
            std::fill(interleaved_dummy_frames.begin(), interleaved_dummy_frames.end(),
                      kDummySampleValue);
        }
    };

    TEST_F(AudioBusTest, FromInterleavedZerosOutUntouchedFrames) {  // NOLINT
        ZeroingOutTestData test_data;

        // Exercise
        test_data.bus_under_test->FromInterleaved<Float32SampleTypeTraits>(
                &test_data.interleaved_dummy_frames[0],
                mm::ZeroingOutTestData::kInterleavedFrameCount);

        // Verification
        for (int ch = 0; ch < mm::ZeroingOutTestData::kChannelCount; ++ch) {
            auto* sample_array_for_current_channel =
                    test_data.bus_under_test->channel(ch);
            for (int frame_index = mm::ZeroingOutTestData::kInterleavedFrameCount;
                 frame_index < mm::ZeroingOutTestData::kFrameCount; frame_index++) {
                ASSERT_EQ(0.0f, sample_array_for_current_channel[frame_index]);
            }
        }
    }

    TEST_F(AudioBusTest, FromInterleavedPartialDoesNotZeroOutUntouchedFrames) { // NOLINT
        {
            SCOPED_TRACE("Zero write offset");

            ZeroingOutTestData test_data;
            static const int kWriteOffsetInFrames = 0;

            // Exercise
            test_data.bus_under_test->FromInterleavedPartial<Float32SampleTypeTraits>(
                    &test_data.interleaved_dummy_frames[0], kWriteOffsetInFrames,
                    mm::ZeroingOutTestData::kInterleavedFrameCount);

            // Verification
            for (int ch = 0; ch < mm::ZeroingOutTestData::kChannelCount; ++ch) {
                auto* sample_array_for_current_channel =
                        test_data.bus_under_test->channel(ch);
                for (int frame_index =
                        mm::ZeroingOutTestData::kInterleavedFrameCount + kWriteOffsetInFrames;
                     frame_index < mm::ZeroingOutTestData::kFrameCount; frame_index++) {
                    ASSERT_EQ(frame_index + 1,
                              sample_array_for_current_channel[frame_index]);
                }
            }
        }
        {
            SCOPED_TRACE("Positive write offset");

            ZeroingOutTestData test_data;
            static const int kWriteOffsetInFrames = 2;

            // Exercise
            test_data.bus_under_test->FromInterleavedPartial<Float32SampleTypeTraits>(
                    &test_data.interleaved_dummy_frames[0], kWriteOffsetInFrames,
                    mm::ZeroingOutTestData::kInterleavedFrameCount);

            // Verification
            for (int ch = 0; ch < mm::ZeroingOutTestData::kChannelCount; ++ch) {
                auto* sample_array_for_current_channel =
                        test_data.bus_under_test->channel(ch);
                // Check untouched frames before write offset
                for (int frame_index = 0; frame_index < kWriteOffsetInFrames;
                     frame_index++) {
                    ASSERT_EQ(frame_index + 1,
                              sample_array_for_current_channel[frame_index]);
                }
                // Check untouched frames after write
                for (int frame_index =
                        mm::ZeroingOutTestData::kInterleavedFrameCount + kWriteOffsetInFrames;
                     frame_index < mm::ZeroingOutTestData::kFrameCount; frame_index++) {
                    ASSERT_EQ(frame_index + 1,
                              sample_array_for_current_channel[frame_index]);
                }
            }
        }
    }

    TEST_F(AudioBusTest, Scale) {   // NOLINT
        std::unique_ptr<AudioBus> bus = AudioBus::Create(kChannels, kFrameCount);

        // Fill the bus with dummy data.
        static const float kFillValue = 1;
        for (int i = 0; i < bus->channels(); ++i)
            std::fill(bus->channel(i), bus->channel(i) + bus->frames(), kFillValue);

        // Adjust by an invalid volume and ensure volume is unchanged.
        bus->Scale(-1);
        for (int i = 0; i < bus->channels(); ++i) {
            SCOPED_TRACE("Invalid Scale");
            VerifyArrayIsFilledWithValue(bus->channel(i), bus->frames(), kFillValue);
        }

        // Verify correct volume adjustment.
        static const float kVolume = 0.5;
        bus->Scale(kVolume);
        for (int i = 0; i < bus->channels(); ++i) {
            SCOPED_TRACE("Half Scale");
            VerifyArrayIsFilledWithValue(bus->channel(i), bus->frames(),
                                         kFillValue * kVolume);
        }

        // Verify zero volume case.
        bus->Scale(0);
        for (int i = 0; i < bus->channels(); ++i) {
            SCOPED_TRACE("Zero Scale");
            VerifyArrayIsFilledWithValue(bus->channel(i), bus->frames(), 0);
        }
    }
}

#pragma clang diagnostic pop