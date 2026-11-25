# Any Batch Queue

This project provides a high-performance, template-based C++ queue component designed to be integrated into pipelines for zero-copy data transfer across subsystems. By leveraging modern C++ move semantics (std::move and rvalue references), it transfers resource ownership directly without unnecessary memory allocations or deep data copying during inter-stage communication.

## 🚀 Key Features

* **Zero-Copy via Move Semantics:** Utilizes C++ move constructors, move assignment operators, and std::move to transfer data ownership between pipeline stages without copying underlying memory buffers.
* **Generic Template Architecture:** Strongly typed, compile-time polymorphic queues allow seamless integration of any movable custom data type, struct, or image matrix (cv::Mat).
* **Thread-Safe Concurrency:** Built-in synchronization mechanisms ensure safe, low-latency, and race-free data flow across multi-threaded producer-consumer subsystems.

## 🛠️ Tech Stack

* **Language:** C++
* **Build System:** CMake
* **OpenCV:** Used for efficient matrix manipulation and zero-copy image buffer handling (cv::Mat move semantics).

## ⚙️ How It Works

1. **Data Ingestion & Allocation:** A producer subsystem captures or generates data (such as a cv::Mat frame or a custom movable object) and allocates it once in memory.
2. **Ownership Transfer via Queue:** Instead of deep-copying the payload, the producer moves ownership (std::move) of the object directly into the type-safe, templated queue.
3. **Subsystem Processing:** Downstream consumer threads pop and move the object out of the queue, processing the data directly from its original memory address before passing it to the next stage.

## 📦 Build Instructions

Ensure that the **OpenCV (4.x)** development library is installed on your system.

```bash
# Clone the repository
git clone https://github.com/hbaltunay/coviso-lab/tree/main/levels/1/any-batch-queue
cd coviso-lab/levels/1/any-batch-queue

# Configure and build the project
cmake -B build
cmake --build build
```

## ▶️ Usage

Once built, run the executable file:

```bash
# Run the demo application to test zero-copy data transfer
./build/bin/queue_app
```
*(Note: Adjust the command-line flags above to match your application's actual argument parser.)*