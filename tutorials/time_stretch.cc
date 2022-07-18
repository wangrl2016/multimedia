//
// Created by admin on 2022/7/18.
//

#include <glog/logging.h>

int main(int argc, char* argv[]) {
    // Initialize Google’s logging library.
    google::InitGoogleLogging(argv[0]);
    FLAGS_stderrthreshold = google::GLOG_INFO;
    if (argc != 3) {
        LOG(ERROR) << "Usage: " << argv[0] << " input_file output_file";
        exit(1);
    }
    return 0;
}
