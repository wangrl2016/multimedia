//
// Created by wangrl2016 on 2022/7/21.
//

#ifndef MULTIMEDIA_MEMORY_PROTOCOL_H
#define MULTIMEDIA_MEMORY_PROTOCOL_H

#include "media/filters/ffmpeg_glue.h"

namespace mm {
    class MemoryProtocol : public FFmpegURLProtocol {
    public:
        MemoryProtocol(const uint8_t* data, int64_t size);

        MemoryProtocol(const MemoryProtocol&) = delete;

        MemoryProtocol& operator=(const MemoryProtocol&) = delete;

        virtual ~MemoryProtocol();

        // FFmpegURLProtocol methods.
        int Read(int size, uint8_t* data) override;

        bool GetPosition(int64_t* position_out) override;

        bool SetPosition(int64_t position) override;

        bool GetSize(int64_t* size_out) override;

    private:
        const uint8_t* data_;
        int64_t size_;
        int64_t position_;
    };
}

#endif //MULTIMEDIA_MEMORY_PROTOCOL_H
