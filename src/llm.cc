#include "llm.hh"
#include <array>
#include <memory>
#include <stdexcept>
#include <fstream>
#include <iostream>
#include <unistd.h>

namespace {

struct PipeDeleter {
    void operator()(FILE* pipe) const {
        if (pipe) {
            pclose(pipe);
        }
    }
};

}

namespace llm {

LLM::LLM(const std::string& api_endpoint,
         const std::string& api_key,
         const std::string& model,
         bool enable_context,
         const std::string& system_prompt)
    : api_endpoint_(api_endpoint),
      api_key_(api_key),
      model_(model),
      enable_context_(enable_context),
      system_prompt_(system_prompt) {
    check_curl_available();
}

std::string LLM::ask(const std::string& question, const std::string& prefix) {
    std::string request_body = build_request_body(question);
    std::string answer = execute_curl_stream(request_body, prefix);

    if (enable_context_) {
        history_.push_back({"user", question});
        history_.push_back({"assistant", answer});
    }

    return answer;
}

void LLM::check_curl_available() {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, PipeDeleter> pipe(popen("which curl", "r"));
    if (!pipe) {
        throw std::runtime_error("Failed to run which curl");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    if (result.empty()) {
        throw std::runtime_error("curl is required but not found in PATH");
    }
}

std::string LLM::build_request_body(const std::string& question) {
    std::string body = "{";
    body += "\"model\":\"" + model_ + "\",";
    body += "\"messages\":[";

    if (!system_prompt_.empty()) {
        body += "{\"role\":\"system\",\"content\":\"" + escape_json(system_prompt_) + "\"},";
    }

    if (enable_context_) {
        for (const auto& msg : history_) {
            body += "{\"role\":\"" + msg.role + "\",\"content\":\"" + escape_json(msg.content) + "\"},";
        }
    }

    body += "{\"role\":\"user\",\"content\":\"" + escape_json(question) + "\"}";
    body += "],";
    body += "\"thinking\":{\"type\":\"enabled\"},";
    body += "\"reasoning_effort\":\"high\",";
    body += "\"stream\":true";
    body += "}";

    return body;
}

std::string LLM::execute_curl_stream(const std::string& request_body, const std::string& prefix) {
    std::string temp_file = "/tmp/llm_request_" + std::to_string(getpid()) + ".json";
    std::ofstream(temp_file) << request_body;

    std::string cmd = "curl -s -N ";
    cmd += api_endpoint_;
    cmd += " -H \"Content-Type: application/json\"";
    cmd += " -H \"Authorization: Bearer " + api_key_ + "\"";
    cmd += " -d @\"" + temp_file + "\"";

    std::array<char, 8192> buffer;
    std::string full_response;
    bool first_chunk = true;
    std::unique_ptr<FILE, PipeDeleter> pipe(popen(cmd.c_str(), "r"));
    if (!pipe) {
        std::remove(temp_file.c_str());
        throw std::runtime_error("Failed to run curl command");
    }

    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        std::string line = buffer.data();
        std::string content = parse_stream_line(line);
        if (!content.empty()) {
            if (first_chunk && !prefix.empty()) {
                std::cout << prefix;
                first_chunk = false;
            }
            std::cout << content << std::flush;
            full_response += content;
        }
    }

    std::cout << std::endl;
    std::remove(temp_file.c_str());
    return full_response;
}

std::string LLM::parse_stream_line(const std::string& line) {
    const std::string data_prefix = "data: ";
    if (line.substr(0, data_prefix.size()) != data_prefix) {
        return "";
    }

    std::string data = line.substr(data_prefix.size());
    while (!data.empty() && (data.back() == '\n' || data.back() == '\r' || data.back() == ' ')) {
        data.pop_back();
    }

    if (data == "[DONE]") {
        return "";
    }

    std::size_t delta_pos = data.find("\"delta\":{");
    if (delta_pos == std::string::npos) {
        return "";
    }

    std::size_t content_pos = data.find("\"content\":\"", delta_pos);
    if (content_pos == std::string::npos) {
        return "";
    }

    std::size_t quote_start = content_pos + 11;
    std::size_t quote_end = quote_start;

    while (quote_end < data.size()) {
        if (data[quote_end] == '\\' && quote_end + 1 < data.size()) {
            quote_end += 2;
        } else if (data[quote_end] == '"') {
            break;
        } else {
            ++quote_end;
        }
    }

    if (quote_end >= data.size()) {
        return "";
    }

    std::string raw_content = data.substr(quote_start, quote_end - quote_start);

    std::string result;
    for (std::size_t i = 0; i < raw_content.size(); ++i) {
        if (raw_content[i] == '\\' && i + 1 < raw_content.size()) {
            switch (raw_content[i + 1]) {
                case 'n': result += '\n'; ++i; break;
                case 't': result += '\t'; ++i; break;
                case 'r': result += '\r'; ++i; break;
                case '\\': result += '\\'; ++i; break;
                case '"': result += '"'; ++i; break;
                default: result += raw_content[i + 1]; ++i; break;
            }
        } else {
            result += raw_content[i];
        }
    }

    return result;
}

std::string LLM::escape_json(const std::string& str) {
    std::string result;
    for (char c : str) {
        switch (c) {
            case '"': result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default: result += c; break;
        }
    }
    return result;
}

}
