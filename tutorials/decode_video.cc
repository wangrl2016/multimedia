//
// Created by wangrl2016 on 2022/7/16.
//

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

int main(int argc, char* argv[]) {
    const char* input_filename, * output_filename;
    const AVCodec* codec;
    AVCodecParserContext* parser;
    AVCodecContext* c = nullptr;
    FILE* file;
    AVPacket* pkt;

    if (argc <= 2) {
        fprintf(stderr, "Usage: %s input_file output_file\n"
                        "And check your input file is encoded by mpeg1video please.\n", argv[0]);
        exit(0);
    }
    input_filename = argv[1];
    output_filename = argv[2];

    pkt = av_packet_alloc();
    if (!pkt)
        exit(1);

    // Set end of buffer to 0 (this ensures that no over reading happens
    // for damaged MPEG streams).


    return 0;
}