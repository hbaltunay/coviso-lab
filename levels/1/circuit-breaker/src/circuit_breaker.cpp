#include <cstdio>

#include "circuit_breaker.hpp"


CircuitBreaker::CircuitBreaker(int failureThreshold, std::chrono::seconds openDuration)
    : state_(State::CLOSED),
      failureCount_(0),
      openedAtNs_(0),
      failureThreshold_(failureThreshold),
      openDuration_(openDuration) {}

int64_t CircuitBreaker::nowNs() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
               std::chrono::steady_clock::now().time_since_epoch()).count();
}

bool CircuitBreaker::allowRequest() {
    switch (state_.load()) {
        case State::CLOSED:
            return true;

        case State::HALF_OPEN:
            return false;

        case State::OPEN: {
            const int64_t elapsedNs = nowNs() - openedAtNs_.load();
            const int64_t durationNs =
                std::chrono::duration_cast<std::chrono::nanoseconds>(openDuration_).count();

            if (elapsedNs < durationNs) {
                return false;
            }

            State expected = State::OPEN;
            if (state_.compare_exchange_strong(expected, State::HALF_OPEN)) {
                printf("Open duration elapsed — transitioning to HALF_OPEN, allowing one trial request.\n");
                return true;
            }
            return false;
        }
    }
    return false;
}

void CircuitBreaker::recordSuccess() {
    State s = state_.load();

    if (s == State::CLOSED) {
        failureCount_.store(0);
        return;
    }

    if (s == State::HALF_OPEN) {
        State expected = State::HALF_OPEN;
        if (state_.compare_exchange_strong(expected, State::CLOSED)) {
            failureCount_.store(0);
            printf("Trial request succeeded — transitioning to CLOSED.\n");
        }
        return;
    }
}

void CircuitBreaker::recordFailure() {
    State s = state_.load();

    if (s == State::HALF_OPEN) {
        State expected = State::HALF_OPEN;
        openedAtNs_.store(nowNs());
        if (state_.compare_exchange_strong(expected, State::OPEN)) {
            printf("Trial request failed in HALF_OPEN — reopening circuit for %lld s.\n",
                   static_cast<long long>(openDuration_.count()));
        }
        return;
    }

    if (s == State::OPEN) {
        return;
    }

    const int count = ++failureCount_;
    if (count >= failureThreshold_) {
        State expected = State::CLOSED;
        openedAtNs_.store(nowNs());
        if (state_.compare_exchange_strong(expected, State::OPEN)) {
            printf("Failure threshold reached (%d failures) — circuit OPEN for %lld s.\n",
                   count, static_cast<long long>(openDuration_.count()));
        }
    }
}

bool CircuitBreaker::isOpen() const {
    return state_.load() == State::OPEN;
}

CircuitBreaker::State CircuitBreaker::getState() const {
    return state_.load();
}

int CircuitBreaker::getFailureCount() const {
    return failureCount_.load();
}