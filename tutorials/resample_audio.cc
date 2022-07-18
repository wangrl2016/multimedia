//
// Created by wangrl2016 on 2022/7/18.
//

#include <glog/logging.h>

int main(int argc, char* argv[]) {
    // Initialize Google’s logging library.
    google::InitGoogleLogging(argv[0]);
    FLAGS_stderrthreshold = google::GLOG_INFO;
    if (argc != 3) {
        LOG(ERROR) << "Usage: " << argv[0] << "output_file";
        LOG(ERROR) << "API example program to show how to resample an audio stream.\n"
                      "This program generates a series of audio frames, resamples them to a specified "
                      "output format and rate and saves them to an output file name output_file.";
        exit(1);
    }
    return 0;
}
