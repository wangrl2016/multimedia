//
// Created by wangrl2016 on 2022/7/21.
//

#ifndef MULTIMEDIA_AUDIO_FILE_READER_H
#define MULTIMEDIA_AUDIO_FILE_READER_H

#include "media/ffmpeg/ffmpeg_deleters.h"
#include "media/filters/ffmpeg_glue.h"

namespace mm {
    class AudioFileReader {
    public:
        explicit AudioFileReader(FFmpegURLProtocol* protocol);
    };

} // mm

#endif //MULTIMEDIA_AUDIO_FILE_READER_H
