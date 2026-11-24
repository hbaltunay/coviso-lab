#pragma once

#include <string>
#include <chrono>
#include <gst/gst.h>
#include <opencv2/core.hpp>


typedef struct _CameraIdMeta {
    GstMeta meta;
    gint cameraId;
} CameraIdMeta;

GType camera_id_meta_api_get_type(void);
#define CAMERA_ID_META_API_TYPE (camera_id_meta_api_get_type())

const GstMetaInfo* camera_id_meta_get_info(void);
#define CAMERA_ID_META_INFO (camera_id_meta_get_info())

CameraIdMeta* gst_buffer_add_camera_id_meta(GstBuffer* buffer, gint cameraId);
CameraIdMeta* gst_buffer_get_camera_id_meta(GstBuffer* buffer);