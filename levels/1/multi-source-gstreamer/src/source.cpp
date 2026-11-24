#include "meta.hpp"
#include "source.hpp"


Source::Source(int cameraId, const std::string& name, const std::string& sourceUri, GstElement* parentPipeline)
    : cameraId_(cameraId),
      name_(name),
      sourceUri_(sourceUri),
      parentPipeline_(parentPipeline),
      decodebin_(nullptr),
      mixerSinkPad_(nullptr) {}

Source::~Source() {
    detach();
}

bool Source::attach() {
    if (!gst_uri_is_valid(sourceUri_.c_str())) return false;

    gchar* protocol = gst_uri_get_protocol(sourceUri_.c_str());
    bool supported = protocol && gst_uri_protocol_is_supported(GST_URI_SRC, protocol);
    isFile_ = protocol && g_str_equal(protocol, "file");
    g_free(protocol);
    if (!supported) return false;

    if (isFile_) {
        gchar* path = g_filename_from_uri(sourceUri_.c_str(), nullptr, nullptr);
        bool ok = path && g_file_test(path, G_FILE_TEST_IS_REGULAR);
        g_free(path);
        if (!ok) return false;
    }

    std::string elementName = "decodebin_" + std::to_string(cameraId_);

    decodebin_ = gst_element_factory_make("uridecodebin", elementName.c_str());

    if (!decodebin_) {
        printf("Decodebin could not be created! Camera Id: %d \n", cameraId_);
        return false;
    }

    g_object_set(G_OBJECT(decodebin_), "uri", sourceUri_.c_str(), NULL);

    gst_element_set_locked_state(decodebin_, TRUE);
    
    g_signal_connect(decodebin_, "pad-added", G_CALLBACK(Source::onPadAdded), this);
    g_signal_connect(decodebin_, "source-setup", G_CALLBACK(Source::onSourceSetup), this);
    
    if (!gst_bin_add(GST_BIN(parentPipeline_), decodebin_)) {
        return false;
    }
    gst_element_set_locked_state(decodebin_, FALSE);

    if (!gst_element_sync_state_with_parent(decodebin_)) {
        return false;
    }

    return true;
}

void Source::detach() {
    if (!decodebin_) return;

    if (parentPipeline_ && mixerSinkPad_) {
        GstElement* mixer = gst_bin_get_by_name(GST_BIN(parentPipeline_), "mixer");
        if (mixer) {
            gst_element_release_request_pad(mixer, mixerSinkPad_);
            gst_object_unref(mixer);
        }
        mixerSinkPad_ = nullptr;
    }

    gst_element_set_locked_state(decodebin_, TRUE);

    gst_element_set_state(decodebin_, GST_STATE_NULL);

    g_signal_handlers_disconnect_by_data(decodebin_, this);

    gst_bin_remove(GST_BIN(parentPipeline_), decodebin_);

    decodebin_ = nullptr;

    printf("Camera (%s) was disconnected from the line. \n", name_.c_str());
}

GstPadProbeReturn Source::stampCameraId(GstPad*, GstPadProbeInfo* info, gpointer userData) {
    Source* self = static_cast<Source*>(userData);

    if (!self || !info || !GST_PAD_PROBE_INFO_BUFFER(info)) {
        return GST_PAD_PROBE_OK;
    }

    GstBuffer* oldBuffer = GST_PAD_PROBE_INFO_BUFFER(info);
    GstBuffer* writableBuffer = gst_buffer_make_writable(oldBuffer);

    if (!writableBuffer) {
        printf("Camera (%s) buffer could not be made writable!\n", self->name_.c_str());
        return GST_PAD_PROBE_OK;
    }

    GST_PAD_PROBE_INFO_DATA(info) = writableBuffer;

    gst_buffer_add_camera_id_meta(writableBuffer, self->cameraId_);

    return GST_PAD_PROBE_OK;
}

GstPadProbeReturn Source::onEosEvent(GstPad* pad, GstPadProbeInfo* info, gpointer userData) {
    GstEvent* event = GST_PAD_PROBE_INFO_EVENT(info);

    if (event && GST_EVENT_TYPE(event) == GST_EVENT_EOS) {
        Source* self = static_cast<Source*>(userData);
        printf("Camera (%s) reached EOS. Processing disconnect.. \n", self->name_.c_str());
        
        if (self->onEosCallback_) {
            int camId = self->cameraId_;
            auto* callbackData = new std::pair<Source*, int>(self, camId);

            g_idle_add([](gpointer data) -> gboolean {
                if (!data) return G_SOURCE_REMOVE;

                auto* pair = static_cast<std::pair<Source*, int>*>(data);
                pair->first->onEosCallback_(pair->second);
                delete pair;
                return G_SOURCE_REMOVE;
            }, callbackData);
        }
        return GST_PAD_PROBE_DROP;
    }
    return GST_PAD_PROBE_OK;
}

void Source::onPadAdded(GstElement *src, GstPad *pad, gpointer userData) {
    Source* sourceObj = static_cast<Source*>(userData);

    GstCaps* caps = gst_pad_query_caps(pad, nullptr);
    if (!caps || gst_caps_is_empty(caps)) {
        if (caps) gst_caps_unref(caps);
        return;
    }

    GstStructure* structure = gst_caps_get_structure(caps, 0);
    const gchar* name = gst_structure_get_name(structure);

    if (g_str_has_prefix(name, "video")) {
        GstElement* pipeline = GST_ELEMENT(gst_element_get_parent(src));
        if (!pipeline) {
            gst_caps_unref(caps);
            return;
        }

        GstClock* clock = gst_element_get_clock(pipeline);
        
        if (clock && !sourceObj->isFile_) {
            GstClockTime now = gst_clock_get_time(clock);
            GstClockTime baseTime = gst_element_get_base_time(pipeline);
            GstClockTime runningTime = now - baseTime;

            gst_pad_set_offset(pad, runningTime);

            printf("Camera (%s) offset applied: %s s \n", sourceObj->name_.c_str(), std::to_string(GST_TIME_AS_SECONDS(runningTime)).c_str());
        
            gst_object_unref(clock);
        }

        gst_pad_add_probe(pad, GST_PAD_PROBE_TYPE_BUFFER, Source::stampCameraId, sourceObj, nullptr);
        gst_pad_add_probe(pad, GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM, Source::onEosEvent, sourceObj, nullptr);

        GstElement* mixer = gst_bin_get_by_name(GST_BIN(pipeline), "mixer");
        if (mixer) {
            GstPad* sinkPad = gst_element_request_pad_simple(mixer, "sink_%u");

            if (sinkPad) {
                if (gst_pad_link(pad, sinkPad) != GST_PAD_LINK_OK) {
                    printf("Camera (%s) could not connect to the mixer!", sourceObj->name_.c_str());
                    gst_element_release_request_pad(mixer, sinkPad);
                } else {
                    printf("Camera (%s) successfully connected to the central line. \n", sourceObj->name_.c_str());
                    sourceObj->mixerSinkPad_ = sinkPad; 
                }
                gst_object_unref(sinkPad);
            }
            gst_object_unref(mixer);
        } else {
            printf("Mixer not found for camera (%s)! \n", sourceObj->name_.c_str());
        }
        gst_object_unref(pipeline);
    }
    gst_caps_unref(caps);
}

void Source::onSourceSetup(GstElement *uridecodebin, GstElement *source, gpointer userData) {
    Source* sourceObj = static_cast<Source*>(userData);

    std::string sourceName = "decSource_" + std::to_string(sourceObj->cameraId_);
    gst_element_set_name(source, sourceName.c_str());

    GstElementFactory *factory = gst_element_get_factory(source);
    const gchar *factoryName = gst_plugin_feature_get_name(GST_PLUGIN_FEATURE(factory));

    if (g_strcmp0(factoryName, "rtspsrc") == 0) {
        g_object_set(G_OBJECT(source),
                     "timeout", (guint64)5000000,
                     "tcp-timeout", (guint64)5000000,
                     "retry", (guint)1,
                     NULL);
    }

    printf("Source setup completed.\n");
}