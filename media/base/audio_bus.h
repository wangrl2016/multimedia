//
// Created by wangrl2016 on 2022/7/19.
//

#ifndef MULTIMEDIA_AUDIO_BUS_H
#define MULTIMEDIA_AUDIO_BUS_H

#include <vector>
#include <memory>
#include "base/memory/aligned_memory.h"

namespace mm {
    class AudioBus {
    public:
        // Guaranteed alignment of each channel's data; use 16-byte alignment for easy
        // SSE optimizations.
        enum {
            kChannelAlignment = 16
        };

        // Create a new AudioBus and allocates |channels| of length |frames|.
        static std::unique_ptr<AudioBus> Create(int channels, int frames);

        // Create a new AudioBus from an existing channel vector. Does not transfer
        // ownership of |channel_data| to AudioBus; i.e., |channel_data| must outlive
        // the returned AudioBus. Each channel must be aligned by kChannelAlignment.
        static std::unique_ptr<AudioBus> WrapVector(
                int frames,
                const std::vector<float*>& channel_data);

        // Creates a new AudioBus by wrapping an existing block of memory. Block must
        // be at least CalculateMemorySize() bytes in size. |data| must outlive the
        // returned AudioBus. |data| must be aligned by kChannelAlignment.
        static std::unique_ptr<AudioBus> WrapMemory(int channels,
                                                    int frames,
                                                    void* data);

        static std::unique_ptr<AudioBus> WrapReadOnlyMemory(int channels,
                                                            int frames,
                                                            const void* data);

        // Based on the given number of channels and frames, calculate the minimum
        // required size in bytes of a contiguous block of memory to be passed to
        // AudioBus for storage of the audio data.
        static int CalculateMemorySize(int channels, int frames);

        // Overwrites the sample value stored in this AudioBus instance with values
        // from a given interleaved |source_buffer| with expected layout
        // [ch0, ch1, ..., chN, ch0, ch1, ...] and sample values in the format
        // corresponding to the given SourceSampleTypeTraits.
        // The sample values are converted to float values by means of the method
        // convert_to_float32() provided by the SourceSampleTypeTraits. For a list of
        // ready-to-use SampleTypeTraits, see file audio_sample_types.h.
        // If |num_frames_to_write| is less than frames(), the remaining frame are
        // zeroed out. If |num_frames_to_write| is more than frames(), this results in
        // undefined behavior.
        template<class SourceSampleTypeTraits>
        void FromInterleaved(
                const typename SourceSampleTypeTraits::Valuetype* source_buffer,
                int num_frames_to_write);

        // Similar to FromInterleaved...(), but overwrites the frames starting at a
        // given offset |write_offset_in_frames| and does not zero out frames that are
        // not overwritten.
        template<class SourceSampleTypeTraits>
        void FromInterleavedPartial(
                const typename SourceSampleTypeTraits::ValueType* source_buffer,
                int write_offset_in_frames,
                int num_frames_to_write);

        // Reads the sample values stored in this AudioBus instance and places them
        // into the given |dest_buffer| in interleaved format using the sample format
        // specified by TargetSampleTypeTraits. For a list of ready-to-use
        // SampleTypeTraits, see file audio_sample_types.h. If |num_frames_to_read| is
        // larger than frames(), this results in undefined behavior.
        template<class TargetSampleTypeTraits>
        void ToInterleaved(
                int num_frames_to_read,
                typename TargetSampleTypeTraits::ValueType* dest_buffer) const;

        // Similar to ToInterleaved(), but reads the frames starting at a given
        // offset |read_offset_in_frames|.
        template<class TargetSampleTypeTraits>
        void ToInterleavedPartial(
                int read_offset_in_frames,
                int num_frames_to_read,
                typename TargetSampleTypeTraits::ValueType* dest_buffer) const;

        // Helper method for copying channel data from one AudioBus to another.  Both
        // AudioBus object must have the same frames() and channels().
        void CopyTo(AudioBus* dest) const;

        // Similar to above, but clips values to [-1, 1] during the copy process.
        void CopyAndClipTo(AudioBus* dest) const;

        // Helper method to copy frames from one AudioBus to another. Both AudioBus
        // objects must have the same number of channels(). |source_start_frame| is
        // the starting offset. |dest_start_frame| is the starting offset in |dest|.
        // |frame_count| is the number of frames to copy.
        void CopyPartialFramesTo(int source_start_frame,
                                 int frame_count,
                                 int dest_start_frame,
                                 AudioBus* dest) const;

        // Returns a raw pointer to the requested channel.  Pointer is guaranteed to
        // have a 16-byte alignment.  Warning: Do not rely on having sane (i.e. not
        // inf, nan, or between [-1.0, 1.0]) values in the channel data.
        float* channel(int channel) { return channel_data_[channel]; }

        const float* channel(int channel) const { return channel_data_[channel]; }

        // Returns the number of channels.
        int channels() const { return static_cast<int>(channel_data_.size()); }

        // Returns the number of frames.
        int frames() const { return frames_; }

        // Helper method for zeroing out all channels of audio data.
        void Zero();

        void ZeroFrames(int frames);

        void ZeroFramesPartial(int start_frame, int frames);

        // Checks if all frames are zero.
        bool AreFramesZero() const;

        // Scale internal channel values by |volume| >= 0.  If an invalid value
        // is provided, no adjustment is done.
        void Scale(float volume);

        // Swaps channels identified by |a| and |b|.  The caller needs to make sure
        // the channels are valid.
        void SwapChannels(int a, int b);

        AudioBus(const AudioBus&) = delete;

        AudioBus& operator=(const AudioBus&) = delete;

        virtual ~AudioBus() = default;

    protected:
        AudioBus(int channels, int frames);

        AudioBus(int channels, int frames, float* data);

        AudioBus(int frames, const std::vector<float*>& channel_data);

    private:
        // Helper method for building |channel_data_| from a block of memory. |data|
        // must be at least CalculateMemorySize(...) bytes in size.
        void BuildChannelData(int channels, int aligned_frame, float* data);

        template<class SourceSampleTypeTraits>
        static void CopyConvertFromInterleavedSourceToAudioBus(
                const typename SourceSampleTypeTraits::ValueType* source_buffer,
                int write_offset_in_frames,
                int num_frames_to_write,
                AudioBus* dest);

        template<class TargetSampleTypeTraits>
        static void CopyConvertFromAudioBusToInterleavedTarget(
                const AudioBus* source,
                int read_offset_in_frames,
                int num_frames_to_read,
                typename TargetSampleTypeTraits::ValueType* dest_buffer);

        // Contiguous block of channel memory.
        std::unique_ptr<float, AlignedFreeDeleter> data_;

        // One float pointer per channel pointing to a contiguous block of memory for
        // that channel. If the memory is owned by this instance, this will
        // point to the memory in |data_|. Otherwise, it may point to memory provided
        // by the client.
        std::vector<float*> channel_data_;
        int frames_;
    };

    // Delegates to FromInterleavedPartial().
    template<class SourceSampleTypeTraits>
    void AudioBus::FromInterleaved(const typename SourceSampleTypeTraits::Valuetype* source_buffer,
                                   int num_frames_to_write) {
        FromInterleavedPartial<SourceSampleTypeTraits>(source_buffer, 0, num_frames_to_write);
        // Zero any remaining frames.
        ZeroFramesPartial(num_frames_to_write, frames_ - num_frames_to_write);
    }

    template<class SourceSampleTypeTraits>
    void AudioBus::FromInterleavedPartial(const typename SourceSampleTypeTraits::ValueType* source_buffer,
                                          int write_offset_in_frames,
                                          int num_frames_to_write) {
        CopyConvertFromInterleavedSourceToAudioBus<SourceSampleTypeTraits>(
                source_buffer,
                write_offset_in_frames,
                num_frames_to_write,
                this);
    }

    template<class TargetSampleTypeTraits>
    void AudioBus::ToInterleaved(int num_frames_to_read,
                                 typename TargetSampleTypeTraits::ValueType* dest_buffer) const {
        ToInterleavedPartial<TargetSampleTypeTraits>(
                0,
                num_frames_to_read,
                dest_buffer);
    }

    template<class TargetSampleTypeTraits>
    void AudioBus::ToInterleavedPartial(int read_offset_in_frames,
                                        int num_frames_to_read,
                                        typename TargetSampleTypeTraits::ValueType* dest_buffer) const {
        CopyConvertFromAudioBusToInterleavedTarget<TargetSampleTypeTraits>(
                this,
                read_offset_in_frames,
                num_frames_to_read,
                dest_buffer);
    }

    template<class SourceSampleTypeTraits>
    void AudioBus::CopyConvertFromInterleavedSourceToAudioBus(
            const typename SourceSampleTypeTraits::ValueType* source_buffer,
            int write_offset_in_frames,
            int num_frames_to_write,
            AudioBus* dest) {
        const int channels = dest->channels();
        for (int ch = 0; ch < channels; ch++) {
            float* channel_data = dest->channel(ch);
            for (int target_frame_index = write_offset_in_frames,
                         read_pos_in_source = ch;
                 target_frame_index < write_offset_in_frames + num_frames_to_write;
                 target_frame_index++, read_pos_in_source += channels) {
                auto source_value = source_buffer[read_pos_in_source];
                channel_data[target_frame_index] =
                        SourceSampleTypeTraits::ToFloat(source_value);
            }
        }
    }

    template<class TargetSampleTypeTraits>
    void AudioBus::CopyConvertFromAudioBusToInterleavedTarget(
            const AudioBus* source,
            int read_offset_in_frames,
            int num_frames_to_read,
            typename TargetSampleTypeTraits::ValueType* dest_buffer) {
        const int channels = source->channels();
        for (int ch = 0; ch < channels; ch++) {
            const float* channel_data = source->channel(ch);
            for (int source_frame_index = read_offset_in_frames, write_pos_in_dest = ch;
                 source_frame_index < read_offset_in_frames + num_frames_to_read;
                 source_frame_index++, write_pos_in_dest += channels) {
                float source_sample_value = channel_data[source_frame_index];
                dest_buffer[write_pos_in_dest] =
                        TargetSampleTypeTraits::FromFloat(source_sample_value);
            }
        }
    }
} // mm

#endif //MULTIMEDIA_AUDIO_BUS_H
