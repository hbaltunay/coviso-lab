# Multi-Source GStreamer

This project enables reading and processing multiple video sources through a single unified pipeline in C++ using GStreamer and OpenCV.

## 🚀 Key Features

* **Unified Multi-Source Pipeline:** Multiple video sources are consolidated and managed within a single GStreamer pipeline (`GstPipeline`) with low overhead, eliminating the need for separate pipelines.
* **Buffer-Level Source Metadata Embedding:** During the frame capture stage, source identification metadata is embedded directly into each frame's `GstBuffer`.
* **Dynamic Processing with OpenCV:** Once frames reach the processing stage, the embedded source metadata is extracted from the `GstBuffer`. Frame data is mapped to a `cv::Mat` object, and targeted image transformations are executed using OpenCV based on the identified source.

## 🛠️ Tech Stack

* **Language:** C++
* **Build System:** CMake
* **GStreamer:** Multimedia pipeline construction, source multiplexing, and `GstBuffer` / `GstMeta` management.
* **OpenCV:** `cv::Mat`-based image transformation, computer vision, and frame processing.

## ⚙️ How It Works

1. **Capture & Metadata Injection:** Frames from multiple video sources enter the GStreamer pipeline, where each frame's `GstBuffer` is tagged with its corresponding source metadata.
2. **Source Consolidation:** All tagged sources travel through a single unified pipeline toward the processing layer.
3. **Extraction & Transformation:** At the processing stage, the source information is retrieved from the `GstBuffer`, the buffer memory is mapped into an OpenCV `cv::Mat` instance, and the required image transformations are applied.

## 📦 Build Instructions

Ensure that **GStreamer (1.0+)** and **OpenCV (4.x)** development libraries are installed on your system.

```bash
# Clone the repository
git clone https://github.com/hbaltunay/coviso-lab/tree/main/levels/1/multi-source-gstreamer
cd coviso-lab/levels/1/multi-source-gstreamer

# Configure and build the project
cmake -B build
cmake --build build
```

## ▶️ Usage

Once built, run the executable by passing multiple sources (e.g., RTSP streams or local video files) into the unified pipeline:

```bash
# Process two different video sources through a single pipeline
./build/bin/multi_source_app --sources "file:///path/to/video1.mp4" "file:///path/to/video2.mp4"
# OR
./build/bin/multi_source_app --sources "rtsp://192.168.1.10:554/stream1" "rtsp://192.168.1.10:554/stream2"
```
*(Note: Adjust the command-line flags above to match your application's actual argument parser.)*