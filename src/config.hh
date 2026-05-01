#ifndef CONFIG_HH
#define CONFIG_HH

#include <string>
#include <unordered_map>

namespace config {

struct Provider {
    std::string api_key;
    std::string base_url;
    std::string model;
    std::string system_prompt;
};

struct Config {
    std::string default_provider;
    std::unordered_map<std::string, Provider> providers;
};

std::string get_config_path();
std::string get_config_dir();
bool file_exists(const std::string& path);
void create_config_template(const std::string& path);
Config load_config(const std::string& path);

} // namespace config

#endif // CONFIG_HH
