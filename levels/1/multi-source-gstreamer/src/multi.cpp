#include <iostream>
#include <gst/video/video.h>

#include "multi.hpp"


MultiSource::MultiSource() {
    init();
}

MultiSource::~MultiSource() {
    // stop();
}

void MultiSource::init() {
    gst_init(nullptr, nullptr);

    loop_     = g_main_loop_new(nullptr, FALSE);
    pipeline_ = gst_pipeline_new("multi-source");

    mixer_      = gst_element_factory_make("funnel", "mixer");
    convert_    = gst_element_factory_make("videoconvert", "converter");
    scaler_     = gst_element_factory_make("videoscale", "scaler");
    capsfilter_ = gst_element_factory_make("capsfilter", "filter");
    appsink_    = gst_element_factory_make("appsink", "sink");

    if (!pipeline_ || !mixer_ || !convert_ || !scaler_ || !capsfilter_ || !appsink_) {
        printf("Basic elements could not be created.!\n");
        return;
    }

    g_object_set(G_OBJECT(appsink_), "emit-signals", TRUE, "max-buffers", 1, NULL);

    GstCaps* caps = gst_caps_from_string("video/x-raw, format=BGR, width=640, height=640");
    g_object_set(G_OBJECT(capsfilter_), "caps", caps, NULL);
    gst_caps_unref(caps);

    gst_bin_add_many(GST_BIN(pipeline_), mixer_, convert_, scaler_, capsfilter_, appsink_, NULL);

    if (!gst_element_link_many(mixer_, convert_, scaler_, capsfilter_, appsink_, NULL)) {
        printf("Basic elements could not be linked!\n");
        return;
    }

    GstBus* bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline_));
    busWatchId_ = gst_bus_add_watch(bus, MultiSource::onBusMessage, this);
    gst_object_unref(bus);

    g_signal_connect(appsink_, "new-sample", G_CALLBACK(MultiSource::onNewSample), this);

    printf("Multi-source has been initialized.\n");
}

int MultiSource::addSource(const std::string& name, const std::string& uri) {
    std::lock_guard<std::mutex> lock(sourceMutex_);

    int newId = nextCameraId_.fetch_add(1);
    
    auto source = std::make_unique<Source>(newId, name, uri, pipeline_);
    
    source->setEosCallback([this](int cameraId) {
        printf("Auto-removing camera %d after EOS.\n", cameraId);
        this->removeSource(cameraId);
    });
    
    if (!source->attach()) {
        source->detach();
        printf("New source could not be added.. Id: %d Name: %s \n", newId, name.c_str());
        return -1;
    }

    activeSources_[newId] = std::move(source);
    
    printf("New source is added independently of the line. Id: %d Name: %s \n", newId, name.c_str());
    
    return newId;
}

void MultiSource::removeSource(int cameraId) {
    std::unique_ptr<Source> sourceToDelete = nullptr;
    {
        std::lock_guard<std::mutex> lock(sourceMutex_);
        auto it = activeSources_.find(cameraId);
        if (it != activeSources_.end()) {
            sourceToDelete = std::move(it->second);
            activeSources_.erase(it);
        }
    }

    if (sourceToDelete) {
        sourceToDelete->detach();
        printf("The source has been removed. Camera Id: %d \n", cameraId);
    } else {
        printf("The source not found. Camera Id: %d \n", cameraId);
    }
}

std::vector<SourceInfo> MultiSource::getActiveSources() {
    std::lock_guard<std::mutex> lock(sourceMutex_);
    std::vector<SourceInfo> result;
    result.reserve(activeSources_.size());
    for (const auto& [id, source] : activeSources_) {
        result.push_back({id, source->getName(), source->getUri()});
    }
    return result;
}

bool MultiSource::start() {
    if (running_.exchange(true)) return false;

    if (loopThread_.joinable()) {
        printf("Multi-source is already running!\n");
        return false;
    }

    if (!pipeline_) init();

    loopThread_ = std::thread(&MultiSource::run, this);

    gst_element_set_state(pipeline_, GST_STATE_PLAYING);

    printf("Multi-source starting..\n");

    return true;
}

void MultiSource::run() {
    g_main_loop_run(loop_);
}


void MultiSource::stop() {
    if (!running_.exchange(false)) return;
    if (loop_ && g_main_loop_is_running(loop_)) g_main_loop_quit(loop_);
    if (loopThread_.joinable()) loopThread_.join();

    std::vector<int> sourceIds;
    {
        std::lock_guard<std::mutex> lock(sourceMutex_);
        for (const auto& [id, source] : activeSources_) {
            sourceIds.push_back(id);
        }
    }
    for (int id : sourceIds) removeSource(id);

    if (busWatchId_ > 0) {
        g_source_remove(busWatchId_);
        busWatchId_ = 0;
    }
    if (pipeline_) {
        gst_element_set_state(pipeline_, GST_STATE_NULL);
        gst_object_unref(pipeline_);
        pipeline_ = nullptr;
    }
    if (loop_) {
        g_main_loop_unref(loop_);
        loop_ = nullptr;
    }

    printf("Multi-source stopped\n");

}

gboolean MultiSource::onBusMessage(GstBus* bus, GstMessage* msg, gpointer userData) {
    MultiSource* self = static_cast<MultiSource*>(userData);

    switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_ERROR: {
            GError* err = nullptr;
            gchar* dbg = nullptr;
            gst_message_parse_error(msg, &err, &dbg);

            const gchar* srcName = GST_MESSAGE_SRC_NAME(msg);
            std::string elementName = srcName ? srcName : "Unknown Element";

            printf("GStreamer Error from [%s]: %s \n", elementName.c_str(), err->message);

            if (self && (g_str_has_prefix(elementName.c_str(), "decodebin_") || 
                g_str_has_prefix(elementName.c_str(), "decSource_"))) {
                try {
                    int camId = std::stoi(elementName.substr(10));
                    printf("Bus: Camera %d encountered a fatal error. Initiating auto-removal..\n", camId);
                    self->removeSource(camId);
                } catch (const std::exception& e) {
                    printf("Bus: Failed to parse camera ID from element name: %s \n", e.what());
                }
            }
            g_error_free(err);
            g_free(dbg);
            break;
        }
        case GST_MESSAGE_WARNING: {
            GError* err = nullptr;
            gchar* dbg = nullptr;
            gst_message_parse_warning(msg, &err, &dbg);

            const gchar* srcName = GST_MESSAGE_SRC_NAME(msg);
            std::string elementName = srcName ? srcName : "Unknown Element";

            printf("GStreamer Warning from [%s]: %s", elementName.c_str(), err->message);

            g_error_free(err);
            g_free(dbg);
            break;
        }
        default:
            break;
    }
    return TRUE;
}

GstFlowReturn MultiSource::onNewSample(GstElement* sink, gpointer userData) {
    MultiSource* receiver = static_cast<MultiSource*>(userData);
    if (!receiver) return GST_FLOW_OK;

    GstSample* sample = nullptr;
    g_signal_emit_by_name(sink, "pull-sample", &sample);
    if (!sample) return GST_FLOW_OK;

    GstCaps* caps = gst_sample_get_caps(sample);
    if (!caps) {
        gst_sample_unref(sample);
        return GST_FLOW_OK;
    }

    GstStructure* structure = gst_caps_get_structure(caps, 0);
    int width = 0, height = 0;
    gst_structure_get_int(structure, "width", &width);
    gst_structure_get_int(structure, "height", &height);

    GstBuffer* buffer = gst_sample_get_buffer(sample);
    if (!buffer) return GST_FLOW_OK;

    GstMapInfo map{};

    cv::Mat frame;

    if (gst_buffer_map(buffer, &map, GST_MAP_READ)) {
        frame = cv::Mat(cv::Size(width, height), CV_8UC3, (char*)map.data, cv::Mat::AUTO_STEP);
    }

    if (frame.empty()) {
        gst_sample_unref(sample);
        return GST_FLOW_OK;
    }

    CameraIdMeta* meta = gst_buffer_get_camera_id_meta(buffer);

    printf("Camera Id: %d | Frame Shape: %d x %d\n", meta->cameraId, frame.rows, frame.cols);

    /*
     *
     *  PROCESS
     *
     */

    gst_buffer_unmap(buffer, &map);
    gst_sample_unref(sample);

    return GST_FLOW_OK;
}