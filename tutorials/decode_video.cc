//
// Created by wangrl2016 on 2022/7/16.
//

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

#define IN_BUF_SIZE 4096

static void PgmSave(unsigned char* buf, int wrap, int width, int height, char* filename) {
    FILE* f;
    int i;

    f = fopen(filename, "wb");
    fprintf(f, "P5\n%d %d\n%d\n", width, height, 255);
    for (i = 0; i < height; i++)
        fwrite(buf + i * wrap, 1, width, f);
    fclose(f);
}

static void Decode(AVCodecContext* dec_ctx, AVFrame* frame, AVPacket* pkt,
                   const char* filename) {
    char buf[1024];
    int ret;

    ret = avcodec_send_packet(dec_ctx, pkt);
    if (ret < 0) {
        fprintf(stderr, "Error sending a packet for decoding\n");
        exit(1);
    }

    while (ret >= 0) {
        ret = avcodec_receive_frame(dec_ctx, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            return;
        else if (ret < 0) {
            fprintf(stderr, "Error during decoding\n");
            exit(1);
        }

        printf("saving frame %3d\n", dec_ctx->frame_number);
        fflush(stdout);

        // The picture is allocated by the decoder. no need to free it.
        snprintf(buf, sizeof(buf), "%s-%d", filename, dec_ctx->frame_number);
        PgmSave(frame->data[0], frame->linesize[0],
                frame->width, frame->height, buf);
    }
}

int main(int argc, char* argv[]) {
    const char* input_filename, * output_filename;
    uint8_t in_buf[IN_BUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE];
    uint8_t* data;
    size_t data_size;
    int eof = false;
    int ret;
    const AVCodec* codec;
    AVCodecParserContext* parser;
    AVCodecContext* codec_ctx = nullptr;
    FILE* file;
    AVPacket* pkt;
    AVFrame* frame;

    if (argc <= 2) {
        fprintf(stderr, "Usage: %s input_file output_file\n"
                        "And check your input file is encoded by mpeg2video please.\n",
                        argv[0]);
        exit(0);
    }
    input_filename = argv[1];
    output_filename = argv[2];

    pkt = av_packet_alloc();
    if (!pkt)
        exit(1);

    // Set end of buffer to 0 (this ensures that no over reading happens
    // for damaged MPEG streams).
    memset(in_buf + IN_BUF_SIZE, 0, AV_INPUT_BUFFER_PADDING_SIZE);

    // find the MPEG-1 video decoder
    codec = avcodec_find_decoder(AV_CODEC_ID_MPEG2VIDEO);
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
        fprintf(stderr, "Could not allocate video codec context\n");
        exit(1);
    }

    // For some codecs, such as ms-mpeg4 and mpeg4, width and height
    // MUST be initialized there because this information is not
    // available in the bitstream.
    if (avcodec_open2(codec_ctx, codec, nullptr) < 0) {
        fprintf(stderr, "Could not open codec\n");
        exit(1);
    }
    file = fopen(input_filename, "rb");
    if (!file) {
        fprintf(stderr, "Could not open %s\n", input_filename);
        exit(1);
    }

    frame = av_frame_alloc();
    if (!frame) {
        fprintf(stderr, "Could not allocate video frame\n");
        exit(1);
    }

    while (!eof) {
        // Read raw data from the input file.
        data_size = fread(in_buf, 1, IN_BUF_SIZE, file);
        if (ferror(file))
            break;
        eof = data_size <= 0;

        // Use the parser to split the data into frames.
        data = in_buf;
        while (data_size > 0 || eof) {
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
                Decode(codec_ctx, frame, pkt, output_filename);
            else if (eof)
                break;

        }
    }

    // Flush  the decoder.
    Decode(codec_ctx, frame, nullptr, output_filename);

    fclose(file);
    av_parser_close(parser);
    avcodec_free_context(&codec_ctx);
    av_frame_free(&frame);
    av_packet_free(&pkt);

    return 0;
}
