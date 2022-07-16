//
// Created by wang rl on 2022/7/16.
//

#include <cstdio>
#include <cstdlib>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/timestamp.h>
}

#include "media/ffmpeg/ffmpeg_util.h"

static uint8_t* video_dst_data[4] = {nullptr};
static int video_dst_line_size[4] = {};
static FILE* video_dst_file, * audio_dst_file;
static int video_dst_buf_size = 0;
static int video_frame_count = 0, audio_frame_count = 0;

static int OutputVideoFrame(AVCodecContext* dec_ctx,
                            AVFrame* frame) {
    if (frame->width != dec_ctx->width ||
        frame->height != dec_ctx->height ||
        frame->format != dec_ctx->pix_fmt) {
        // To handle this change, one could call av_image_alloc again and
        // decode the following frames into another raw video file.
        fprintf(stderr, "Error: Width, height and pixel format have to be "
                        "constant in a raw video file, but the width, height or "
                        "pixel format of the input video changed:\n"
                        "old: width = %d, height = %d, format = %s\n"
                        "new: width = %d, height = %d, format = %s\n",
                dec_ctx->width, dec_ctx->height, av_get_pix_fmt_name(dec_ctx->pix_fmt),
                frame->width, frame->height,
                av_get_pix_fmt_name(static_cast<AVPixelFormat>(frame->format)));
        return -1;
    }

    printf("video_frame n:%d coded_n: %d\n",
           video_frame_count++,
           frame->coded_picture_number);

    // Copy decoded frame to destination buffer:
    // this is required since raw video expects non-aligned data.
    av_image_copy(video_dst_data,
                  video_dst_line_size,
                  (const uint8_t**) (frame->data),
                  frame->linesize,
                  dec_ctx->pix_fmt,
                  dec_ctx->width,
                  dec_ctx->height);

    // Write to raw video file.
    fwrite(video_dst_data[0], 1, video_dst_buf_size, video_dst_file);
    return 0;
}

static int OutputAudioFrame(AVCodecContext* dec_ctx,
                            AVFrame* frame) {
    size_t un_padded_line_size =
            frame->nb_samples * av_get_bytes_per_sample(static_cast<AVSampleFormat>(frame->format));
    printf("audio_frame n:%d nb_samples:%d pts:%s\n",
           audio_frame_count++, frame->nb_samples,
           av_ts2timestr(frame->pts, &dec_ctx->time_base));

    // Write the raw audio data samples of the first plane. This works
    // fine for packed formats (e.g. AV_SAMPLE_FMT_S16). However,
    // most audio decoders output planar audio, which uses a separate
    // plane of audio samples for each channel (e.g. AV_SAMPLE_FMT_S16P).
    // In other words, this code will write only the first audio channel
    // in these cases.
    // You should use libswresample or libavfilter to convert the frame
    // to packed data.
    fwrite(frame->extended_data[0], 1, un_padded_line_size, audio_dst_file);

    return 0;
}

static int DecodePacket(AVCodecContext* dec_ctx,
                        const AVPacket* pkt,
                        AVFrame* frame) {
    // submit the packet to the decoder
    int ret = avcodec_send_packet(dec_ctx, pkt);
    if (ret < 0) {
        fprintf(stderr, "Error submitting a packet for decoding (%s)\n", av_err2str(ret));
        return ret;
    }

    // Get all the available frames from the decoder.
    while (ret >= 0) {
        ret = avcodec_receive_frame(dec_ctx, frame);
        if (ret < 0) {
            // Those two return values are special and mean there is no output
            // frame available, but there were no errors during decoding.
            if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN))
                return 0;

            fprintf(stderr, "Error during decoding (%s)\n", av_err2str(ret));
            return ret;
        }

        // Write the frame data to output file.
        if (dec_ctx->codec->type == AVMEDIA_TYPE_VIDEO) {
            ret = OutputVideoFrame(dec_ctx, frame);
        } else
            ret = OutputAudioFrame(dec_ctx, frame);

        av_frame_unref(frame);
        if (ret < 0)
            return ret;
    }
    return 0;
}

static int OpenCodecContext(AVFormatContext* fmt_ctx,
                            AVCodecContext** dec_ctx,
                            int* stream_idx,
                            enum AVMediaType type) {
    int stream_index;
    AVStream* stream;
    const AVCodec* decoder;
    int ret = av_find_best_stream(fmt_ctx, type, -1,
                                  -1, nullptr, 0);
    if (ret < 0) {
        fprintf(stderr, "Could not find %s stream in input file\n",
                av_get_media_type_string(type));
        return ret;
    } else {
        stream_index = ret;
        stream = fmt_ctx->streams[stream_index];

        // find decoder for the stream
        decoder = avcodec_find_decoder(stream->codecpar->codec_id);
        if (!decoder) {
            fprintf(stderr, "Failed to find %s codec\n",
                    av_get_media_type_string(type));
            return AVERROR(EINVAL);
        }

        // Allocate a codec context for the decoder.
        *dec_ctx = avcodec_alloc_context3(decoder);
        if (!*dec_ctx) {
            fprintf(stderr, "Failed to allocate the %s codec context\n",
                    av_get_media_type_string(type));
            return AVERROR(ENOMEM);
        }

        // Copy codec parameters from input stream to output codec context.
        if ((ret = avcodec_parameters_to_context(*dec_ctx, stream->codecpar)) < 0) {
            fprintf(stderr, "Failed to copy %s codec parameters to decoder context\n",
                    av_get_media_type_string(type));
            return ret;
        }

        // Init the decoders.
        if ((ret = avcodec_open2(*dec_ctx, decoder, nullptr)) < 0) {
            fprintf(stderr, "Failed to open %s codec\n",
                    av_get_media_type_string(type));
            return ret;
        }
        *stream_idx = stream_index;
    }
    return 0;
}

int main(int argc, char* argv[]) {
    int ret = 0;
    const char* input_filename;
    const char* video_dst_filename, * audio_dst_filename;
    int video_stream_idx = -1, audio_stream_idx = -1;

    AVFormatContext* fmt_ctx = nullptr;
    AVCodecContext* video_dec_ctx = nullptr, * audio_dec_ctx = nullptr;
    AVStream* video_stream = nullptr, * audio_stream = nullptr;
    AVFrame* frame = nullptr;
    AVPacket* pkt = nullptr;

    if (argc != 4) {
        fprintf(stderr, "Usage: %s input_file video_output_file audio_output_file\n"
                        "API example program to show how to read frames from an input file.\n"
                        "This program reads frames from a file, decodes them, and writes decoded\n"
                        "video frames to a raw-video file named video_output_file, and decoded\n"
                        "audio frames to a raw-audio file named audio_output_file.\n",
                argv[0]);
        exit(1);
    }

    input_filename = argv[1];
    video_dst_filename = argv[2];
    audio_dst_filename = argv[3];

    // Open input file, and allocate format context.
    if (avformat_open_input(&fmt_ctx, input_filename, nullptr, nullptr) < 0) {
        fprintf(stderr, "Could not open source file %s\n", input_filename);
        exit(1);
    }

    // Retrieve stream information.
    if (avformat_find_stream_info(fmt_ctx, nullptr) < 0) {
        fprintf(stderr, "Could not find stream information\n");
        exit(1);
    }

    if (OpenCodecContext(fmt_ctx,
                         &video_dec_ctx,
                         &video_stream_idx,
                         AVMEDIA_TYPE_VIDEO) >= 0) {
        video_stream = fmt_ctx->streams[video_stream_idx];
        video_dst_file = fopen(video_dst_filename, "wb");
        if (!video_dst_file) {
            fprintf(stderr, "Could not open destination file %s\n", video_dst_filename);
            ret = 1;
            goto end;
        }

        // allocate image where the decoded image will be put
        ret = av_image_alloc(video_dst_data, video_dst_line_size,
                             video_dec_ctx->width,
                             video_dec_ctx->height,
                             video_dec_ctx->pix_fmt,
                             1);
        if (ret < 0) {
            fprintf(stderr, "Could not allocate raw video buffer\n");
            goto end;
        }
        video_dst_buf_size = ret;
    }

    if (OpenCodecContext(fmt_ctx,
                         &audio_dec_ctx,
                         &audio_stream_idx,
                         AVMEDIA_TYPE_AUDIO) >= 0) {
        audio_stream = fmt_ctx->streams[audio_stream_idx];
        audio_dst_file = fopen(audio_dst_filename, "wb");
        if (!audio_dst_file) {
            fprintf(stderr, "Could not open destination file %s\n", audio_dst_filename);
            ret = 1;
            goto end;
        }
    }

    // dump input information to stderr
    av_dump_format(fmt_ctx, 0, input_filename, 0);

    if (!audio_stream && !video_stream) {
        fprintf(stderr, "Could not find audio or video stream in the input, aborting\n");
        ret = 1;
        goto end;
    }

    frame = av_frame_alloc();
    if (!frame) {
        fprintf(stderr, "Could not allocate frame\n");
        ret = AVERROR(ENOMEM);
        goto end;
    }

    pkt = av_packet_alloc();
    if (!pkt) {
        fprintf(stderr, "Could not allocate packet\n");
        ret = AVERROR(ENOMEM);
        goto end;
    }

    if (video_stream)
        printf("Demux video from file '%s' into '%s'\n",
               input_filename, video_dst_filename);
    if (audio_stream)
        printf("Demux audio from file '%s' into '%s'\n",
               input_filename, audio_dst_filename);

    // read frames from the file
    while (av_read_frame(fmt_ctx, pkt) >= 0) {
        // Check if the packet belongs to a stream we are interested in, otherwise skip it.
        if (pkt->stream_index == video_stream_idx)
            ret = DecodePacket(video_dec_ctx, pkt, frame);
        else if (pkt->stream_index == audio_stream_idx)
            ret = DecodePacket(audio_dec_ctx, pkt, frame);
        av_packet_unref(pkt);
        if (ret < 0)
            break;
    }

    // flush the decoders
    if (video_dec_ctx)
        DecodePacket(video_dec_ctx, nullptr, frame);
    if (audio_dec_ctx)
        DecodePacket(audio_dec_ctx, nullptr, frame);

    if (video_stream) {
        printf("Play the output video file with the command:\n"
               "ffplay -f rawvideo -pix_fmt %s -video_size %dx%d %s\n",
               av_get_pix_fmt_name(video_dec_ctx->pix_fmt),
               video_dec_ctx->width,
               video_dec_ctx->height,
               video_dst_filename);
    }

    if (audio_stream) {
        enum AVSampleFormat sample_fmt = audio_dec_ctx->sample_fmt;
        int n_channels = audio_dec_ctx->channels;
        const char* fmt;
        if (av_sample_fmt_is_planar(sample_fmt)) {
            const char* pack = av_get_sample_fmt_name(sample_fmt);
            printf("Warning: the sample format the decoder producer is planar "
                   "(%s). This example will output the first channel only.\n",
                   pack ? pack : "?");
            sample_fmt = av_get_packed_sample_fmt(sample_fmt);
            n_channels = 1;
        }
        if ((ret = mm::GetFormatFromSampleFmt(&fmt, sample_fmt)) < 0)
            goto end;

        printf("Play the output audio file with the command:\n"
               "ffplay -f %s -ac %d -ar %d %s\n",
               fmt,
               n_channels,
               audio_dec_ctx->sample_rate,
               audio_dst_filename);
    }

    end:
    if (video_dec_ctx)
        avcodec_free_context(&video_dec_ctx);
    if (audio_dec_ctx)
        avcodec_free_context(&audio_dec_ctx);
    avformat_close_input(&fmt_ctx);
    if (video_dst_file)
        fclose(video_dst_file);
    if (audio_dst_file)
        fclose(audio_dst_file);
    av_packet_free(&pkt);
    av_frame_free(&frame);
    av_free(video_dst_data[0]);
    return ret < 0;
}
