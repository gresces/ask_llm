#include "config.hh"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <sstream>
#include <sys/stat.h>

namespace config {

std::string get_config_dir() {
    const char* home = std::getenv("HOME");
    if (!home) {
        throw std::runtime_error("HOME environment variable is not set");
    }
    return std::string(home) + "/.config/ask";
}

std::string get_config_path() {
    return get_config_dir() + "/config";
}

bool file_exists(const std::string& path) {
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
}

void create_config_template(const std::string& path) {
    std::string dir = path.substr(0, path.find_last_of('/'));
    
    std::string mkdir_cmd = "mkdir -p " + dir;
    int ret = system(mkdir_cmd.c_str());
    if (ret != 0) {
        throw std::runtime_error("Failed to create config directory: " + dir);
    }
    
    std::ofstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to create config file at " + path);
    }
    
    file << "# LLM 配置文件模板\n";
    file << "# 配置文件格式: INI-like\n";
    file << "# default 指定默认使用的 provider\n\n";
    file << "default = deepseek\n\n";
    file << "[deepseek]\n";
    file << "api_key = sk-xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n";
    file << "base_url = https://api.deepseek.com/chat/completions\n";
    file << "model = deepseek-chat\n";
    file << "# 系统提示词，设定 AI 的角色和行为\n";
    file << "# 使用 \\n 表示换行，例如：system_prompt = 你是一个专家\n";
    file << "system_prompt = 你是一个有帮助的 AI 助手。请用简洁、准确的中文回答问题。\n\n";
    file << "# 其他 provider 示例:\n";
    file << "# [openai]\n";
    file << "# api_key = sk-xxxxxxxx\n";
    file << "# base_url = https://api.openai.com/v1/chat/completions\n";
    file << "# model = gpt-4\n";
    
    file.close();
    
    std::cout << "\n========================================\n";
    std::cout << "配置文件已创建\n";
    std::cout << "位置: " << path << "\n";
    std::cout << "\n请编辑配置文件，填入你的 API key\n";
    std::cout << "\n使用示例:\n";
    std::cout << "  ./bin/ask \"你好\"\n";
    std::cout << "========================================\n\n";
}

Config load_config(const std::string& path) {
    if (!file_exists(path)) {
        try {
            create_config_template(path);
        } catch (const std::exception& e) {
            std::cerr << "\n⚠️  警告: 无法创建配置文件\n";
            std::cerr << "错误: " << e.what() << "\n";
            std::cerr << "请手动创建配置文件: " << path << "\n\n";
            throw;
        }
        
        throw std::runtime_error("请编辑配置文件: " + path + " 并填写 API key");
    }
    
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open configuration file at " + path);
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
            } else if (key == "system_prompt") {
                provider.system_prompt = value;
            }
        }
    }
    
    return config;
}

}
