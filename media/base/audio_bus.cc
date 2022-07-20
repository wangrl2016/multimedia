//
// Created by wangrl2016 on 2022/7/19.
//

#include "media/base/audio_bus.h"
#include "media/base/audio_sample_types.h"

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

    std::unique_ptr<AudioBus> AudioBus::WrapVector(int frames,
                                                   const std::vector<float*>& channel_data) {
        return std::unique_ptr<AudioBus>(new AudioBus(frames, channel_data));
    }

    std::unique_ptr<AudioBus> AudioBus::WrapMemory(int channels,
                                                   int frames,
                                                   void* data) {
        // |data| must be aligned by AudioBus::kChannelAlignment.
        CHECK(IsAligned(data));
        return std::unique_ptr<AudioBus>(new AudioBus(channels, frames, static_cast<float*>(data)));
    }

    std::unique_ptr<AudioBus> AudioBus::WrapReadOnlyMemory(int channels,
                                                           int frames,
                                                           const void* data) {
        // Note: const_cast is generally dangerous but is used in this case since
        // AudioBus accommodates both read-only and read/write use cases. A const
        // AudioBus object is returned to ensure no one accidentally writes to the
        // read-only data.
        return WrapMemory(channels, frames, const_cast<void*>(data));
    }

    int AudioBus::CalculateMemorySize(int channels, int frames) {
        return CalculateMemorySizeInternal(channels, frames, nullptr);
    }

    void AudioBus::CopyTo(AudioBus* dest) const {
        CopyPartialFramesTo(0, frames(), 0, dest);
    }

    void AudioBus::CopyAndClipTo(AudioBus* dest) const {
        CHECK_EQ(channels(), dest->channels());
        CHECK_LE(frames(), dest->frames());
        for (int i = 0; i < channels(); i++) {
            float* dest_ptr = dest->channel(i);
            const float* source_ptr = channel(i);
            for (int j = 0; j < frames(); j++)
                dest_ptr[j] = Float32SampleTypeTraits::FromFloat(source_ptr[j]);
        }
    }

    void AudioBus::CopyPartialFramesTo(int source_start_frame,
                                       int frame_count,
                                       int dest_start_frame,
                                       AudioBus* dest) const {
        CHECK_EQ(channels(), dest->channels());
        CHECK_LE(source_start_frame + frame_count, frames());
        CHECK_LE(dest_start_frame + frame_count, dest->frames());

        // Since we don't know if the other AudioBus is wrapped or not (and we don't
        // want to care), just copy using the public channel() accessors.
        for (int i = 0; i < channels(); ++i) {
            memcpy(dest->channel(i) + dest_start_frame,
                   channel(i) + source_start_frame,
                   sizeof(*channel(i)) * frame_count);
        }
    }

    void AudioBus::Zero() {
        ZeroFrames(frames_);
    }

    void AudioBus::ZeroFrames(int frames) {
        ZeroFramesPartial(0, frames);
    }

    void AudioBus::ZeroFramesPartial(int start_frame, int frames) {
        CheckOverflow(start_frame, frames, frames_);
        if (frames <= 0)
            return;

        for (size_t i = 0; i < channel_data_.size(); ++i) {
            memset(channel_data_[i] + start_frame, 0,
                   frames * sizeof(*channel_data_[i]));
        }
    }

    bool AudioBus::AreFramesZero() const {
        for (size_t i = 0; i < channel_data_.size(); ++i) {
            for (int j = 0; j < frames_; ++j) {
                if (channel_data_[i][j])
                    return false;
            }
        }
        return true;
    }

    void AudioBus::Scale(float volume) {
        if (volume > 0 && volume != 1) {
            for (int i = 0; i < channels(); i++) {
                // TODO: Use SSE or NEON accelerate
                float* data = channel(i);
                for (int j = 0; j < frames(); j++)
                    data[j] = data[j] * volume;
            }
        } else if (volume == 0) {
            Zero();
        }
    }

    void AudioBus::SwapChannels(int a, int b) {
        DCHECK(a < channels() && a >= 0);
        DCHECK(b < channels() && b >= 0);
        DCHECK_NE(a, b);
        std::swap(channel_data_[a], channel_data_[b]);
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
        for (auto& i: channel_data_)
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
