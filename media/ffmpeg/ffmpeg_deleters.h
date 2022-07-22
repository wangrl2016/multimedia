//
// Created by wangrl2016 on 2022/7/21.
//

#ifndef MULTIMEDIA_FFMPEG_DELETERS_H
#define MULTIMEDIA_FFMPEG_DELETERS_H

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
};

namespace mm {
    struct AVIOContextFree {
        void operator()(void* x) const;
    };
}

#endif //MULTIMEDIA_FFMPEG_DELETERS_H
