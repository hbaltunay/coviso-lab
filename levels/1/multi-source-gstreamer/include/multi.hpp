#pragma once

#include <atomic>
#include <cstdio>
#include <map>
#include <mutex>
#include <thread>
#include <vector>
#include <gst/gst.h>

#include "meta.hpp"
#include "source.hpp"


struct SourceInfo {
    int camera_id;
    std::string name;
    std::string uri;
};

class MultiSource {
private:
    guint busWatchId_;

    std::map<int, std::unique_ptr<Source>> activeSources_;
    std::mutex sourceMutex_;

    std::atomic<bool> running_{false};
    std::atomic<int> nextCameraId_{1};
    std::thread loopThread_;

    // GStreamer Plugins
    GstElement *pipeline_;
    GstElement *mixer_;
    GstElement *convert_;
    GstElement *scaler_;
    GstElement *capsfilter_;
    GstElement *appsink_;
    GMainLoop  *loop_;

    void run();
    static GstFlowReturn onNewSample(GstElement *sink, gpointer userData);
    static gboolean onBusMessage(GstBus *bus, GstMessage *msg, gpointer userData);

public:
    MultiSource();
    ~MultiSource();

    bool start();
    void stop();
    void init();

    int addSource(const std::string& name, const std::string& uri);
    void removeSource(int cameraId);
    std::vector<SourceInfo> getActiveSources();

    GstElement* getMixerBin() { return mixer_; }
};