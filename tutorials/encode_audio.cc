//
// Created by wang rl on 2022/7/15.
//

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

static bool CheckSampleFmt(const AVCodec* codec, enum AVSampleFormat sample_format) {
    const enum AVSampleFormat* p = codec->sample_fmts;

    while (*p != AV_SAMPLE_FMT_NONE) {
        if (*p == sample_format)
            return true;
        p++;
    }
    return false;
}

int main(int argc, char* argv[]) {
    const char* filename;
    const AVCodec* codec;
    AVCodecContext* codec_ctx = nullptr;
    AVFrame* frame;
    AVPacket* pkt;
    int i, j, k, ret;
    FILE* file;
    uint16_t* samples;
    float t, incr;

    if (argc <= 1) {
        fprintf(stderr, "Usage: %s output_file\n", argv[0]);
        return 0;
    }
    filename = argv[1];

    // find the MP2 encoder
    codec = avcodec_find_encoder(AV_CODEC_ID_MP2);
    if (!codec) {
        fprintf(stderr, "Codec not found\n");
        exit(1);
    }

    codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx) {
        fprintf(stderr, "Could not allocate audio codec context\n");
        exit(1);
    }

    // put sample parameters
    codec_ctx->bit_rate = 64000;

    // Check that the encoder supports s16 pcm input.
    codec_ctx->sample_fmt = AV_SAMPLE_FMT_S16;


    return 0;
}