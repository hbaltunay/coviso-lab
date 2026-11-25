#include <cstdio>
#include <thread>
#include <chrono>

#include "queue.hpp"


void producer(Queue<std::string>& queue) {
    int c = 0;
    std::string pName;

    while (true) {
        pName.clear();
        pName.reserve(16); 

        pName += "Process-";
        pName += std::to_string(c);

        const void* adrs = static_cast<void*>(pName.data());
        
        printf("[%p] Name: %s (Total Process: %ld)\n",
               adrs, pName.c_str(), queue.size());
        
        queue.push(std::move(pName));

        if (c == 9) break;
        c++;

        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }
}

void consumer(Queue<std::string>& queue) {
    std::string pName;

    while (queue.pop(pName)) {
        const void* adrs = static_cast<void*>(pName.data());

        printf("[%p] Name: %s (Remaining Process: %ld)\n", 
               adrs, pName.c_str(), queue.size());

        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

int main() {
    Queue<std::string> q;
    std::thread ct(consumer, std::ref(q));
    std::thread pt(producer, std::ref(q));

    pt.join();
    ct.join();
    
    return 0;
}