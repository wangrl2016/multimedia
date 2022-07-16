//
// Created by wang rl on 2022/7/16.
//

#include "media/ffmpeg/ffmpeg_util.h"

namespace mm {
    int GetFormatFromSampleFmt(const char** fmt, enum AVSampleFormat sample_fmt) {
        int i;
        struct sample_fmt_entry {
            enum AVSampleFormat sample_fmt;
            const char* fmt_be, * fmt_le;
        } sample_fmt_entries[] = {
                {AV_SAMPLE_FMT_U8,  "u8",    "u8"},
                {AV_SAMPLE_FMT_S16, "s16be", "s16le"},
                {AV_SAMPLE_FMT_S32, "s32be", "s32le"},
                {AV_SAMPLE_FMT_FLT, "f32be", "f32le"},
                {AV_SAMPLE_FMT_DBL, "f64be", "f64le"},
        };
        *fmt = nullptr;

        for (i = 0; i < FF_ARRAY_ELEMS(sample_fmt_entries); i++) {
            struct sample_fmt_entry* entry = &sample_fmt_entries[i];
            if (sample_fmt == entry->sample_fmt) {
                *fmt = AV_NE(entry->fmt_be, entry->fmt_le);
                return 0;
            }
        }

        fprintf(stderr, "sample format %s is not supported as output format\n",
                av_get_sample_fmt_name(sample_fmt));
        return -1;
    }
}