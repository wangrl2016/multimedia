//
// Created by wangrl2016 on 2022/7/13.
//

#include <glog/logging.h>

int main(int argc, char* argv[]) {
    // Initialize Google’s logging library.
    google::InitGoogleLogging(argv[0]);
    FLAGS_stderrthreshold = google::GLOG_INFO;

    LOG(INFO) << "Found " << argc << " cookies";
}