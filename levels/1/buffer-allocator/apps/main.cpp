#include <cstdio>
#include <gst/gst.h>
#include <opencv2/core.hpp>

#include "allocator.hpp"


GstSample* createFrameSample(int width, int height, int channels = 3) {
    const gsize bufferSize = width * height * channels;

    GstBuffer* buffer = gst_buffer_new_allocate(nullptr, bufferSize, nullptr);
    
    GstMapInfo map;
    if (gst_buffer_map(buffer, &map, GST_MAP_WRITE)) {
        for (gsize i = 0; i < bufferSize; i += 3) {
            map.data[i]     = 255;
            map.data[i + 1] = 0;
            map.data[i + 2] = 0;
        }
        gst_buffer_unmap(buffer, &map);
    }

    GstCaps* caps = gst_caps_new_simple("video/x-raw",
                                        "format", G_TYPE_STRING, "BGR",
                                        "width", G_TYPE_INT, width,
                                        "height", G_TYPE_INT, height,
                                        "framerate", GST_TYPE_FRACTION, 30, 1,
                                        nullptr);

    GstSample* sample = gst_sample_new(buffer, caps, nullptr, nullptr);

    gst_buffer_unref(buffer);
    gst_caps_unref(caps);

    return sample;
}


int main(int argc, char *argv[]) {
    gst_init(&argc, &argv);

    int width = 32, height = 32;

    GstSample* sample = createFrameSample(width, height);

    cv::Mat frame = GstBufferMatAllocator::wrap(sample, width, height, CV_8UC3);

    if (frame.empty()) {
        return -1;
    }

    printf("Image Shape: %d x %d \n", frame.cols, frame.rows);

    return 0;
}