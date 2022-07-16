//
// Created by wangrl2016 on 2022/7/16.
//

#ifndef MULTIMEDIA_FFMPEG_UTIL_H
#define MULTIMEDIA_FFMPEG_UTIL_H

extern "C" {
#include <libavutil/samplefmt.h>
}

namespace mm {
    int GetFormatFromSampleFmt(const char** fmt,
                               enum AVSampleFormat sample_fmt);
}

#endif //MULTIMEDIA_FFMPEG_UTIL_H
