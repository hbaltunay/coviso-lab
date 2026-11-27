# Circuit Breaker

This project is a lightweight Circuit Breaker pattern implementation written in C++ designed to prevent cascading failures in fault-tolerant applications. It monitors function execution via try-catch blocks, trips after a configurable number of consecutive failures, and automatically handles cooldown periods and recovery probes.

## 🚀 Key Features

* **Configurable Failure Threshold:** Automatically trips the circuit breaker after a specified number (X) of consecutive exceptions are caught.
* **Timed Cooldown & Recovery:** Enters an open state for a user-defined duration before allowing a single test request to check if the underlying service has recovered.
* **Automatic State Management:** Seamlessly transitions between Closed, Open, and Half-Open states, resetting the failure counter upon a successful recovery probe or restarting the cooldown timer if the probe fails.

## 🛠️ Tech Stack

* **Language:** C++
* **Build System:** CMake

## ⚙️ How It Works

1. **Closed State (Normal Operation):** Incoming operations are executed within a try-catch block. If an exception is thrown X consecutive times (based on the failure threshold parameter), the circuit trips and transitions to the Open state.
2. **Open State (Cooldown):** All incoming requests are immediately blocked without execution for a specified timeout duration to give the failing system time to recover.
3. **Half-Open State (Recovery Probe):** Once the cooldown timer expires, the next incoming operation is allowed through as a single test attempt. If it succeeds, the circuit resets to Closed; if it fails, the circuit immediately returns to the Open state and restarts the timer.

## 📦 Build Instructions

Ensure your system has the C++ Standard Library available (no third-party libraries needed).

```bash
# Clone the repository
git clone https://github.com/hbaltunay/coviso-lab/tree/main/levels/1/circuit-breaker
cd coviso-lab/levels/1/circuit-breaker

# Configure and build the project
cmake -B build
cmake --build build
```

## ▶️ Usage

Once built, run the executable app:

```bash
# Run the demo application for circuit breaker
./build/bin/circuit_breaker_app
```
*(Note: Adjust the command-line flags above to match your application's actual argument parser.)*