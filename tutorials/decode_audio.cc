//
// Created by wangrl2016 on 2022/7/15.
//

#include <cstdio>
#include <cstdlib>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

#include "media/ffmpeg/ffmpeg_util.h"

#define AUDIO_BUF_SIZE 20480
#define AUDIO_REFILL_THRESH 4096

static void Decode(AVCodecContext* dec_ctx,
                   AVPacket* pkt,
                   AVFrame* frame,
                   FILE* out_file) {
    int i, ch;
    int ret, data_size;

    // Send the packet with the compressed data to the decoder.
    ret = avcodec_send_packet(dec_ctx, pkt);
    if (ret < 0) {
        fprintf(stderr, "Error submitting the packet to the decoder\n");
        exit(1);
    }

    // Real all the output frames (in general there may be any number of them
    while (ret >= 0) {
        ret = avcodec_receive_frame(dec_ctx, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            return;
        else if (ret < 0) {
            fprintf(stderr, "Error during decoding\n");
            exit(1);
        }
        data_size = av_get_bytes_per_sample(dec_ctx->sample_fmt);
        if (data_size < 0) {
            // This should not occur, checking just for paranoia.
            fprintf(stderr, "Failed to calculate data size\n");
            exit(1);
        }
        for (i = 0; i < frame->nb_samples; i++)
            for (ch = 0; ch < dec_ctx->channels; ch++)
                fwrite(frame->data[ch] + data_size * i, 1, data_size, out_file);
    }
}

int main(int argc, char* argv[]) {
    const char* input_filename, * output_filename;
    FILE* input_file, * output_file;
    uint8_t* data;
    size_t data_size;
    uint8_t buf[AUDIO_BUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE];
    int n_channels;
    const char* fmt;
    size_t len;
    int ret;

    const AVCodec* codec;
    AVCodecParserContext* parser;
    AVCodecContext* codec_ctx = nullptr;
    AVPacket* pkt = nullptr;
    AVFrame* decoded_frame = nullptr;
    enum AVSampleFormat sample_fmt;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s input_file output_file\n", argv[0]);
        exit(0);
    }
    input_filename = argv[1];
    output_filename = argv[2];

    pkt = av_packet_alloc();

    // find the MPEG audio decoder
    codec = avcodec_find_decoder(AV_CODEC_ID_MP2);
    if (!codec) {
        fprintf(stderr, "Codec not found\n");
        exit(1);
    }

    parser = av_parser_init(codec->id);
    if (!parser) {
        fprintf(stderr, "Parser not found\n");
        exit(1);
    }

    codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx) {
        fprintf(stderr, "Could not allocate audio codec context\n");
        exit(1);
    }

    // open it
    if (avcodec_open2(codec_ctx, codec, nullptr) < 0) {
        fprintf(stderr, "Could not open codec\n");
        exit(1);
    }

    input_file = fopen(input_filename, "rb");
    if (!input_file) {
        fprintf(stderr, "Could not open %s\n", input_filename);
        exit(1);
    }

    output_file = fopen(output_filename, "wb");
    if (!output_file) {
        av_free(codec_ctx);
        exit(1);
    }

    // decode until eof
    data = buf;
    data_size = fread(buf, 1, AUDIO_BUF_SIZE, input_file);

    while (data_size > 0) {
        if (!(decoded_frame = av_frame_alloc())) {
            fprintf(stderr, "Could not allocate audio frame\n");
            exit(1);
        }

        ret = av_parser_parse2(parser,
                               codec_ctx,
                               &pkt->data,
                               &pkt->size,
                               data,
                               int(data_size),
                               AV_NOPTS_VALUE,
                               AV_NOPTS_VALUE,
                               0);
        if (ret < 0) {
            fprintf(stderr, "Error while parsing\n");
            exit(1);
        }
        data += ret;
        data_size -= ret;

        if (pkt->size)
            Decode(codec_ctx, pkt, decoded_frame, output_file);

        if (data_size < AUDIO_REFILL_THRESH) {
            memmove(buf, data, data_size);
            data = buf;
            len = fread(data + data_size, 1,
                        AUDIO_BUF_SIZE - data_size, input_file);
            if (len > 0)
                data_size += len;
        }
    }

    // flush the decoder
    pkt->data = nullptr;
    pkt->size = 0;
    Decode(codec_ctx, pkt, decoded_frame, output_file);

    // Print output pcm information, because there have no metadata of pcm.
    sample_fmt = codec_ctx->sample_fmt;

    if (av_sample_fmt_is_planar(sample_fmt)) {
        const char* packed = av_get_sample_fmt_name(sample_fmt);
        printf("Warning: the sample format the decoder produced is planar (%s).\n",
               packed ? packed : "?");
        sample_fmt = av_get_packed_sample_fmt(sample_fmt);
    }

    n_channels = codec_ctx->channels;
    if ((ret = mm::GetFormatFromSampleFmt(&fmt, sample_fmt)) < 0) {
        fprintf(stderr, "%d\n", ret);
        goto end;
    }

    printf("Play the output audio file with the command:\n"
           "ffplay -f %s -ac %d -ar %d %s\n",
           fmt, n_channels, codec_ctx->sample_rate,
           output_filename);
    end:
    fclose(output_file);
    fclose(input_file);

    avcodec_free_context(&codec_ctx);
    av_parser_close(parser);
    av_frame_free(&decoded_frame);
    av_packet_free(&pkt);

    return 0;
}
