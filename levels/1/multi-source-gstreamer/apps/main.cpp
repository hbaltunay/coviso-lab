#include <atomic>
#include <csignal>

#include "multi.hpp"
#include "util.hpp"

static std::atomic<bool> g_run{true};

static void on_signal(int) {
    g_run.store(false);
}


int main(int argc, char* argv[]) {
    std::vector<std::string> uris = UriHelper::parseAndFormatSources(argc, argv);

    if (uris.empty()) {
        return -1;
    }

    std::signal(SIGINT,  on_signal);
    std::signal(SIGTERM, on_signal);

    MultiSource source;
    source.start();

    for (size_t i = 0; i < uris.size(); ++i) {
        std::string name = "Test" + std::to_string(i+1);
        source.addSource(name, uris[i]);
    }

    while (g_run) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    source.stop();
    
    return 0;
}