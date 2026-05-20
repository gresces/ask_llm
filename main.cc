#include "src/llm.hh"
#include "src/config.hh"
#include <iostream>
#include <string>

namespace {

std::string build_translation_system_prompt(const std::string& mode) {
    if (mode == "tEn") {
        return "You are a translation engine. Detect whether the user's text is primarily Chinese or English. "
               "If it is Chinese, translate it into natural English. If it is English, translate it into natural Chinese. "
               "Return only the translation, without explanations, alternatives, quotes, or extra commentary.";
    }

    if (mode == "tZh") {
        return "You are a translation engine. Translate the user's text into natural Chinese. "
               "Return only the translation, without explanations, alternatives, quotes, or extra commentary.";
    }

    if (mode == "tJp") {
        return "You are a translation engine. Translate the user's text into natural Japanese. "
               "Return only the translation, without explanations, alternatives, quotes, or extra commentary.";
    }

    return "You are a translation engine. Translate the user's text into natural Japanese. "
           "Return only the translation, without explanations, alternatives, quotes, or extra commentary.";
}

std::string join_args(int argc, char* argv[], int start) {
    std::string result;
    for (int i = start; i < argc; ++i) {
        if (!result.empty()) {
            result += " ";
        }
        result += argv[i];
    }
    return result;
}

}

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

        std::string mode = argc > 1 ? argv[1] : "";
        bool translate_mode = mode == "tEn" || mode == "tZh" || mode == "tJp";
        llm::LLM llm(provider.base_url,
                     provider.api_key,
                     provider.model,
                     !translate_mode,
                     translate_mode ? build_translation_system_prompt(mode) : provider.system_prompt,
                     translate_mode ? 0 : -1);

        std::string input;

        std::string prefix = cfg.default_provider + ": ";

        if (translate_mode) {
            input = join_args(argc, argv, 2);
            prefix = "translate: ";
        } else if (argc > 1) {
            input = join_args(argc, argv, 1);
        }

        if (!input.empty()) {
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
