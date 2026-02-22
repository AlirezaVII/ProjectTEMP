#include "dotenv.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>

void load_dotenv(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        // Silently fail if file doesn't exist, common practice for .env loaders
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        // Skip empty lines or comments
        if (line.empty() || line[0] == '#') continue;

        size_t delimiterPos = line.find('=');
        if (delimiterPos != std::string::npos) {
            std::string key = line.substr(0, delimiterPos);
            std::string value = line.substr(delimiterPos + 1);

            // Trim potential whitespace around key/value
            auto trim = [](std::string& s) {
                s.erase(0, s.find_first_not_of(" \t\r\n"));
                s.erase(s.find_last_not_of(" \t\r\n") + 1);
            };
            trim(key);
            trim(value);

            // Set the environment variable globally
            #ifdef _WIN32
                _putenv_s(key.c_str(), value.c_str());
            #else
                setenv(key.c_str(), value.c_str(), 1);
            #endif
        }
    }
    file.close();
}