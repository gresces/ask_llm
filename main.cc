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

        if (provider.api_key.empty()) {
            std::cerr << "Error: api_key is not set for provider '"
                      << cfg.default_provider << "'." << std::endl;
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

        llm::LLM llm(provider.base_url, provider.api_key, provider.model, true);

        std::string input;

        if (argc > 1) {
            input = argv[1];
            std::string answer = llm.ask(input);
            std::cout << cfg.default_provider << ": " << answer << std::endl;
        }

        while (true) {
            std::getline(std::cin, input);

            if (input.empty()) {
                break;
            }

            std::string answer = llm.ask(input);
            std::cout << cfg.default_provider << ": " << answer << std::endl;
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
