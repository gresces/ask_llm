#include "src/llm.hh"
#include "src/config.hh"
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    try {
        std::string config_path = config::get_config_path();
        config::Config cfg = config::load_config(config_path);

        if (cfg.default_provider.empty()) {
            std::cerr << "Error: No default provider specified in config." << std::endl;
            return 1;
        }

        auto it = cfg.providers.find(cfg.default_provider);
        if (it == cfg.providers.end()) {
            std::cerr << "Error: Default provider '" << cfg.default_provider
                      << "' not found in config." << std::endl;
            return 1;
        }

        const config::Provider& provider = it->second;

        if (provider.api_key.empty() || provider.api_key.find("sk-xxxxxxxx") == 0) {
            std::cerr << "Error: api_key is not set or still using placeholder for provider '"
                      << cfg.default_provider << "'." << std::endl;
            std::cerr << "Please edit your config file: " << config_path << std::endl;
            return 1;
        }

        if (provider.base_url.empty()) {
            std::cerr << "Error: base_url is not set for provider '"
                      << cfg.default_provider << "'." << std::endl;
            return 1;
        }

        if (provider.model.empty()) {
            std::cerr << "Error: model is not set for provider '"
                      << cfg.default_provider << "'." << std::endl;
            return 1;
        }

        llm::LLM llm(provider.base_url, provider.api_key, provider.model, true, provider.system_prompt);

        std::string input;

        std::string prefix = cfg.default_provider + ": ";

        if (argc > 1) {
            input = argv[1];
            llm.ask(input, prefix);
        }

        while (true) {
            std::getline(std::cin, input);

            if (input.empty()) {
                break;
            }

            llm.ask(input, prefix);
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
