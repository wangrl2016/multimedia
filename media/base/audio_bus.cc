//
// Created by wangrl2016 on 2022/7/19.
//

#include "media/base/audio_bus.h"

namespace mm {
    static bool IsAligned(void* ptr) {
        return (reinterpret_cast<uintptr_t>(ptr) & (AudioBus::kChannelAlignment - 1)) == 0U;
    }

    // In order to guarantee that the memory block for each channel starts at an
    // aligned address when splitting a contiguous block of memory into one block
    // per channel, we may have to make these blocks larger than otherwise needed.
    // We do this by allocating space for potentially more frames than requested.
    // This method returns the required size for the contiguous memory block
    // in bytes and outputs the adjusted number of frames via |out_aligned_frames|.
    static int CalculateMemorySizeInternal(int channels,
                                           int frames,
                                           int* out_aligned_frames) {
        // Since our internal sample format is float, we can guarantee the alignment
        // by making the number of frames an integer multiply of
        // AudioBus::kChannelAlignment / sizeof(float).
        int aligned_frames =
                ((frames * sizeof(float) + AudioBus::kChannelAlignment - 1) &
                 ~(AudioBus::kChannelAlignment - 1)) / sizeof(float);

        if (out_aligned_frames)
            *out_aligned_frames = aligned_frames;

        return sizeof(float) * channels * aligned_frames;
    }

    static void ValidateConfig(int channels, int frames) {
        CHECK_GT(frames, 0);
        CHECK_GT(channels, 0);
    }

    static void CheckOverflow(int start_frame, int frames, int total_frames) {
        CHECK_GE(start_frame, 0);
        CHECK_GE(frames, 0);
        CHECK_GT(total_frames, 0);
        int sum = start_frame + frames;
        CHECK_LE(sum, total_frames);
        CHECK_GE(sum, 0);
    }

    std::unique_ptr<AudioBus> AudioBus::Create(int channels, int frames) {
        return std::unique_ptr<AudioBus>(new AudioBus(channels, frames));
    }

    AudioBus::AudioBus(int channels, int frames) : frames_(frames) {
        ValidateConfig(channels, frames_);

        int aligned_frames = 0;
        int size = CalculateMemorySizeInternal(channels, frames, &aligned_frames);

        data_.reset(static_cast<float*>(AlignedAlloc(
                size, AudioBus::kChannelAlignment)));

        BuildChannelData(channels, aligned_frames, data_.get());
    }

    AudioBus::AudioBus(int channels, int frames, float* data) : frames_(frames) {
        // Since |data| may have come from an external source, ensure it's valid.
        CHECK(data);
        ValidateConfig(channels, frames_);

        int aligned_frames = 0;
        CalculateMemorySizeInternal(channels, frames, &aligned_frames);

        BuildChannelData(channels, aligned_frames, data);
    }

    AudioBus::AudioBus(int frames, const std::vector<float*>& channel_data) :
            channel_data_(channel_data), frames_(frames) {

        // Sanity check wrapped vector for alignment and channel count.
        for (auto & i : channel_data_)
            DCHECK(IsAligned(i));
    }

    void AudioBus::BuildChannelData(int channels, int aligned_frame, float* data) {
        DCHECK(IsAligned(data));
        DCHECK_EQ(channel_data_.size(), 0U);
        // Initialize |channel_data_| with pointers into |data|.
        channel_data_.reserve(channels);
        for (int i = 0; i < channels; i++)
            channel_data_.push_back(data + i * aligned_frame);
    }
} // mm
