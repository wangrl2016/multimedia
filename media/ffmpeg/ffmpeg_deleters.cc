//
// Created by wang rl on 2022/7/21.
//

#include "media/ffmpeg/ffmpeg_deleters.h"

namespace mm {
    inline void AVIOContextFree::operator()(void* x) const {
        auto* context = static_cast<AVIOContext*>(x);
        if (context)
            av_freep(&context->buffer);
        avio_context_free(&context);
    }
}
