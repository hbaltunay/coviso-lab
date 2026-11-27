#include <cstdio>
#include <chrono>
#include <stdexcept>
#include <thread>

#include "circuit_breaker.hpp"


void process() {

    int ctrl = 0;
    CircuitBreaker circuitBreaker(3, std::chrono::seconds(2));

    while (ctrl < 15) {
        ctrl++;

        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        printf("[%d] Process:\n", ctrl);

        if (!circuitBreaker.allowRequest()) {
            printf("Circuit open — dropping process without attempting.\n");
            continue;
        }

        try {
            if (ctrl < 5 || ctrl > 10) {
                throw std::runtime_error("Condition failed.\n");
            }
            circuitBreaker.recordSuccess();
            printf("Process completed.\n");

        } catch (const std::runtime_error& e) {
            printf("Error caught: %s", e.what());
            circuitBreaker.recordFailure();

        } catch (...) {
            printf("An unknown error occurred!\n");
            circuitBreaker.recordFailure();
        }
    }
}


int main() {

    process();

    return 0;
}