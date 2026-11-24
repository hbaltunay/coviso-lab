#include <gst/gst.h>
#include <filesystem>
#include <iostream>

#include "util.hpp"

namespace fs = std::filesystem;


std::string UriHelper::formatToGstreamerUri(const std::string& input) {
    if (gst_uri_is_valid(input.c_str())) {
        return input;
    }

    std::error_code ec;
    fs::path absPath = fs::weakly_canonical(fs::path(input), ec);
    std::string finalPath = ec ? input : absPath.string();

    GError* error = nullptr;
    gchar* uri = g_filename_to_uri(finalPath.c_str(), nullptr, &error);
    
    if (error != nullptr) {
        printf("URI creation error (%s): %s \n", input.c_str(), error->message);
        g_error_free(error);
        return "";
    }

    std::string result(uri);
    g_free(uri);
    return result;
}

std::vector<std::string> UriHelper::parseAndFormatSources(int& argc, char**& argv) {
    gchar** raw_sources = nullptr;
    GError* error = nullptr;
    std::vector<std::string> uris;

    GOptionEntry entries[] = {
        { "source", 's', 0, G_OPTION_ARG_STRING_ARRAY, &raw_sources, "Video files or RTSP sources", "URI_OR_PATH" },
        { nullptr }
    };

    GOptionContext* ctx = g_option_context_new("- Multi-Source GStreamer Application");
    g_option_context_add_main_entries(ctx, entries, nullptr);
    g_option_context_add_group(ctx, gst_init_get_option_group());

    if (!g_option_context_parse(ctx, &argc, &argv, &error)) {
        std::cerr << "Parameter error: " << error->message << std::endl;
        g_error_free(error);
        g_option_context_free(ctx);
        return uris;
    }
    g_option_context_free(ctx);

    if (raw_sources != nullptr) {
        for (int i = 0; raw_sources[i] != nullptr; ++i) {
            std::string formatted = formatToGstreamerUri(raw_sources[i]);
            if (!formatted.empty()) {
                uris.push_back(formatted);
            }
        }
        g_strfreev(raw_sources);
    }

    for (int i = 1; i < argc; ++i) {
        std::string formatted = formatToGstreamerUri(argv[i]);
        if (!formatted.empty()) {
            uris.push_back(formatted);
        }
    }

    if (uris.empty()) {
        printf("Usage: %s --source <src1> --source <src2> ... \n", argv[0]);
    }

    return uris;
}