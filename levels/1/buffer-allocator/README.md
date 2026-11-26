# Buffer Allocator

This project demonstrates zero-copy conversion of video frames captured via GStreamer into OpenCV cv::Mat objects using a custom cv::MatAllocator. By mapping GStreamer buffer memory directly and managing its lifecycle through OpenCV's internal reference counting, it eliminates expensive memory copies during real-time video processing.

## 🚀 Key Features

* **True Zero-Copy Pipeline:** Directly maps GStreamer GstBuffer memory into cv::Mat headers without invoking memcpy.
* **Custom cv::MatAllocator:** Wraps external GStreamer buffer memory instead of allocating new heap memory, handling resource cleanup automatically inside deallocate() when the buffer is no longer needed.
* **Safe Ownership & Reference Counting:** Ties GstSample and GstBuffer lifecycles to OpenCV's UMatData reference counter, ensuring memory is safely unmapped and unreferenced only when all cv::Mat views are destroyed.

## 🛠️ Tech Stack

* **Language:** C++
* **Build System:** CMake
* **GStreamer:** Captures video frames from the media pipeline, extracts samples via appsink, and provides direct low-level access to raw frame buffers without extra copies.
* **OpenCV:** Wraps raw external memory into cv::Mat headers and manages buffer lifecycles safely through custom cv::MatAllocator and reference counting mechanisms.

## ⚙️ How It Works

1. **Sample Extraction & Memory Mapping:** Frames are pulled from a GStreamer pipeline via appsink as a GstSample. The underlying GstBuffer is mapped (gst_buffer_map) to obtain a raw data pointer along with frame dimensions and stride info.
2. **Custom cv::Mat Wrapping:** A custom cv::MatAllocator creates a cv::UMatData instance pointing directly to the mapped GStreamer memory, storing the GstSample and GstMapInfo within the allocator's user data.
3. **Lifecycle & Reference Management:** The resulting cv::Mat shares ownership across copies and sub-matrices (ROIs). Once the reference count drops to zero, the custom allocator's deallocate method unmaps the buffer (gst_buffer_unmap) and invokes gst_sample_unref to clean up GStreamer resources.

## 📦 Build Instructions

Ensure that **GStreamer (1.0+)** and **OpenCV (4.x)** development libraries are installed on your system.

```bash
# Clone the repository
git clone https://github.com/hbaltunay/coviso-lab/tree/main/levels/1/buffer-allocator
cd coviso-lab/levels/1/buffer-allocator

# Configure and build the project
cmake -B build
cmake --build build
```

## ▶️ Usage

Once built, run the executable app:

```bash
# Run the demo application to test converting a GstBuffer to a cv::Mat
./build/bin/buffer_allocator_app
```
*(Note: Adjust the command-line flags above to match your application's actual argument parser.)*