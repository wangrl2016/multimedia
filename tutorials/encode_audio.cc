//
// Created by wang rl on 2022/7/15.
//

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/channel_layout.h>
}

static void Encode(AVCodecContext* ctx, AVFrame* frame,
                   AVPacket* pkt, FILE* file) {
    // Set the frame for encoding.
    int ret = avcodec_send_frame(ctx, frame);
    if (ret < 0) {
        fprintf(stderr, "Error sending the frame to the encoder\n");
        exit(1);
    }

    // read all the available output packets (in general there may be any
    // number of them
    while (ret >= 0) {
        ret = avcodec_receive_packet(ctx, pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            return;
        else if (ret < 0) {
            fprintf(stderr, "Error encoding audio frame\n");
            exit(1);
        }
        fwrite(pkt->data, 1, pkt->size, file);
        av_packet_unref(pkt);
    }
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
    codec_ctx->sample_rate = 44100;
    codec_ctx->channel_layout = AV_CH_LAYOUT_STEREO;

    // open it
    if (avcodec_open2(codec_ctx, codec, nullptr) < 0) {
        fprintf(stderr, "Could not open codec\n");
        exit(1);
    }

    file = fopen(filename, "wb");
    if (!file) {
        fprintf(stderr, "Could not open %s\n", filename);
        exit(1);
    }

    // packet for holding encoded output.
    pkt = av_packet_alloc();
    if (!pkt) {
        fprintf(stderr, "Could not allocate the packet\n");
        exit(1);
    }

    // Frame containing input raw audio.
    frame = av_frame_alloc();
    if (!frame) {
        fprintf(stderr, "Could not allocate audio frame\n");
        exit(1);
    }

    frame->nb_samples = codec_ctx->frame_size;
    frame->format = codec_ctx->sample_fmt;
    frame->channel_layout = codec_ctx->channel_layout;

    // Allocate the data buffers.
    ret = av_frame_get_buffer(frame, 0);
    if (ret < 0) {
        fprintf(stderr, "Could not allocate audio data buffers\n");
        exit(1);
    }

    // Encode a single tone sound.
    t = 0;
    incr = float(2 * M_PI * 440.0 / codec_ctx->sample_rate);
    for (i = 0; i < 200; i++) {
        // Make sure the frame is writable -- makes a copy if the encoder
        // kept a reference internally.
        ret = av_frame_make_writable(frame);
        if (ret < 0)
            exit(1);
        samples = (uint16_t*) frame->data[0];

        for (j = 0; j < codec_ctx->frame_size; j++) {
            samples[2 * j] = (int) (sin(t) * 10000);

            for (k = 1; k < codec_ctx->channels; k++)
                samples[2 * j + k] = samples[2 * j];
            t += incr;
        }
        Encode(codec_ctx, frame, pkt, file);
    }

    // flush the encoder
    Encode(codec_ctx, nullptr, pkt, file);
    fclose(file);

    av_frame_free(&frame);
    av_packet_free(&pkt);
    avcodec_free_context(&codec_ctx);

    return 0;
}
