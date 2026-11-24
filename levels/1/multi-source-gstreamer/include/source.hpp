#pragma once

#include <string>
#include <gst/gst.h>
#include <functional>


class Source {
private:
    int cameraId_;
    std::string name_;
    std::string sourceUri_;
    bool isFile_;

    GstElement *decodebin_;
    GstElement *parentPipeline_;
    GstPad* mixerSinkPad_;

    std::function<void(int)> onEosCallback_;

    static void onPadAdded(GstElement *src, GstPad *pad, gpointer userData);
    static void onSourceSetup(GstElement *uridecodebin, GstElement *source, gpointer userData);
    static GstPadProbeReturn stampCameraId(GstPad* pad, GstPadProbeInfo* info, gpointer userData);
    static GstPadProbeReturn onEosEvent(GstPad* pad, GstPadProbeInfo* info, gpointer userData);

public:
    Source(int camera_id, const std::string &name, const std::string &source_uri, GstElement *pipeline);
    ~Source();
    
    bool attach();
    void detach();

    void setEosCallback(std::function<void(int)> cb) { onEosCallback_ = cb; }

    int getId() const { return cameraId_; }
    const std::string& getName() const { return name_; }
    const std::string& getUri() const { return sourceUri_; }
};