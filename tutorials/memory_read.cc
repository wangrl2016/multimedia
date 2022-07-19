//
// Created by wangrl2016 on 2022/7/19.
//

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libavutil/file.h>
}

struct BufferData {
    uint8_t* ptr;
    size_t size; // size left in the buffer
};

static int read_packet(void* opaque, uint8_t* buf, int buf_size) {
    auto* bd = (struct BufferData*) opaque;
    buf_size = FFMIN(buf_size, bd->size);

    if (!buf_size)
        return AVERROR_EOF;
    printf("ptr:%p size:%zu\n", bd->ptr, bd->size);

    // copy internal buffer data to buf
    memcpy(buf, bd->ptr, buf_size);
    bd->ptr += buf_size;
    bd->size -= buf_size;

    return buf_size;
}

int main(int argc, char* argv[]) {
    AVFormatContext* fmt_ctx = nullptr;
    AVIOContext* io_ctx = nullptr;
    uint8_t* buffer = nullptr, * io_ctx_buffer;
    size_t buffer_size, io_ctx_buffer_size = 4096;
    char* input_filename;
    int ret;
    struct BufferData bd = {nullptr};

    if (argc != 2) {
        fprintf(stderr, "usage: %s input_file\n"
                        "API example program to show how to read from a custom buffer "
                        "accessed through AVIOContext.\n", argv[0]);
        return 1;
    }
    input_filename = argv[1];

    // slurp file content into buffer
    ret = av_file_map(input_filename,
                      &buffer, &buffer_size, 0, nullptr);
    if (ret < 0)
        goto end;

    // fill opaque structure used by the AVIOContext read callback
    bd.ptr = buffer;
    bd.size = buffer_size;

    if (!(fmt_ctx = avformat_alloc_context())) {
        ret = AVERROR(ENOMEM);
        goto end;
    }

    io_ctx_buffer = static_cast<uint8_t*>(av_malloc(io_ctx_buffer_size));
    if (!io_ctx_buffer) {
        ret = AVERROR(ENOMEM);
        goto end;
    }
    io_ctx = avio_alloc_context(io_ctx_buffer,
                                int(io_ctx_buffer_size),
                                0,
                                &bd,
                                &read_packet,
                                nullptr,
                                nullptr);
    if (!io_ctx) {
        ret = AVERROR(ENOMEM);
        goto end;
    }
    fmt_ctx->pb = io_ctx;

    ret = avformat_open_input(&fmt_ctx, nullptr, nullptr, nullptr);
    if (ret < 0) {
        fprintf(stderr, "Could not open input\n");
        goto end;
    }

    ret = avformat_find_stream_info(fmt_ctx, nullptr);
    if (ret < 0) {
        fprintf(stderr, "Could not find stream information\n");
        goto end;
    }

    av_dump_format(fmt_ctx, 0, input_filename, 0);

    end:
    avformat_close_input(&fmt_ctx);

    // note: the internal buffer could have changed, and be != io_ctx_buffer
    if (io_ctx)
        av_freep(&io_ctx->buffer);
    avio_context_free(&io_ctx);

    av_file_unmap(buffer, buffer_size);

    if (ret < 0) {
        fprintf(stderr, "Error occurred: %s\n", av_err2str(ret));
        return 1;
    }

    return 0;
}
