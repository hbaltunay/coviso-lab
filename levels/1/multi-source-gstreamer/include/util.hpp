#pragma once

#include <string>
#include <vector>


class UriHelper {
public:
    static std::vector<std::string> parseAndFormatSources(int& argc, char**& argv);

    static std::string formatToGstreamerUri(const std::string& input);
};