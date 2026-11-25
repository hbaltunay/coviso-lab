#pragma once

#include <string>
#include <chrono>
#include <opencv2/core.hpp>


struct FrameMeta {
    int camera_id;
    uint64_t frame_num;
    bool in_frame;

    cv::Mat frame;

    std::chrono::steady_clock::time_point timestamp;
};

struct SourceMeta {
    int source_id;
    int camera_id;
    std::string name;
    std::string uri;
};
