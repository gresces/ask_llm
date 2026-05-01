#ifndef CONFIG_HH
#define CONFIG_HH

#include <string>
#include <unordered_map>

namespace config {

struct Provider {
    std::string api_key;
    std::string base_url;
    std::string model;
};

struct Config {
    std::string default_provider;
    std::unordered_map<std::string, Provider> providers;
};

std::string get_config_path();
Config load_config(const std::string& path);

} // namespace config

#endif // CONFIG_HH
