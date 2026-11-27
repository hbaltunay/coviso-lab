#pragma once

#include <atomic>
#include <chrono>


class CircuitBreaker {
public:
    enum class State { CLOSED, OPEN, HALF_OPEN };

    explicit CircuitBreaker(int failureThreshold = 10,
                            std::chrono::seconds openDuration = std::chrono::seconds(30));

    bool allowRequest();

    void recordSuccess();
    void recordFailure();

    bool isOpen() const;
    State getState() const;
    int getFailureCount() const;

private:
    static int64_t nowNs();

    std::atomic<State> state_;
    std::atomic<int> failureCount_;
    std::atomic<int64_t> openedAtNs_;

    const int failureThreshold_;
    const std::chrono::seconds openDuration_;
};