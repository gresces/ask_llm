#include "config.hh"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <sstream>

namespace config {

std::string get_config_path() {
    const char* home = std::getenv("HOME");
    if (!home) {
        throw std::runtime_error("HOME environment variable is not set");
    }
    return std::string(home) + "/.config/ask/config";
}

Config load_config(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Configuration file not found at " + path);
    }

    Config config;
    std::string line;
    std::size_t line_num = 0;
    std::string current_section;

    while (std::getline(file, line)) {
        ++line_num;

        std::size_t start = line.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) {
            continue;
        }
        std::size_t end = line.find_last_not_of(" \t\r\n");
        std::string trimmed = line.substr(start, end - start + 1);

        if (trimmed.empty() || trimmed[0] == '#' || trimmed[0] == ';') {
            continue;
        }

        if (trimmed.front() == '[' && trimmed.back() == ']') {
            current_section = trimmed.substr(1, trimmed.size() - 2);
            continue;
        }

        std::size_t eq_pos = trimmed.find('=');
        if (eq_pos == std::string::npos) {
            throw std::runtime_error(
                "Parse error at line " + std::to_string(line_num) +
                ": expected 'key = value' format, got: " + trimmed
            );
        }

        std::string key = trimmed.substr(0, eq_pos);
        std::string value = trimmed.substr(eq_pos + 1);

        key = key.substr(key.find_first_not_of(" \t"));
        key = key.substr(0, key.find_last_not_of(" \t") + 1);
        value = value.substr(value.find_first_not_of(" \t"));
        value = value.substr(0, value.find_last_not_of(" \t") + 1);

        if (current_section.empty()) {
            if (key == "default") {
                config.default_provider = value;
            }
        } else {
            Provider& provider = config.providers[current_section];
            if (key == "api_key") {
                provider.api_key = value;
            } else if (key == "base_url") {
                provider.base_url = value;
            } else if (key == "model") {
                provider.model = value;
            }
        }
    }

    return config;
}

}
