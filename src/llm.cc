#include "llm.hh"
#include <array>
#include <memory>
#include <stdexcept>
#include <fstream>
#include <unistd.h>

namespace {

struct PipeDeleter {
    void operator()(FILE* pipe) const {
        if (pipe) {
            pclose(pipe);
        }
    }
};

std::size_t find_matching_brace(const std::string& str, std::size_t start) {
    int depth = 0;
    for (std::size_t i = start; i < str.size(); ++i) {
        if (str[i] == '{') ++depth;
        else if (str[i] == '}') {
            --depth;
            if (depth == 0) return i;
        }
    }
    return std::string::npos;
}

}

namespace llm {

LLM::LLM(const std::string& api_endpoint,
         const std::string& api_key,
         const std::string& model,
         bool enable_context)
    : api_endpoint_(api_endpoint),
      api_key_(api_key),
      model_(model),
      enable_context_(enable_context) {
    check_curl_available();
}

std::string LLM::ask(const std::string& question) {
    std::string request_body = build_request_body(question);
    std::string response = execute_curl(request_body);
    std::string answer = parse_response(response);

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

    if (enable_context_) {
        for (const auto& msg : history_) {
            body += "{\"role\":\"" + msg.role + "\",\"content\":\"" + escape_json(msg.content) + "\"},";
        }
    }

    body += "{\"role\":\"user\",\"content\":\"" + escape_json(question) + "\"}";
    body += "],";
    body += "\"thinking\":{\"type\":\"enabled\"},";
    body += "\"reasoning_effort\":\"high\",";
    body += "\"stream\":false";
    body += "}";

    return body;
}

std::string LLM::execute_curl(const std::string& request_body) {
    std::string temp_file = "/tmp/llm_request_" + std::to_string(getpid()) + ".json";
    std::ofstream(temp_file) << request_body;

    std::string cmd = "curl -s ";
    cmd += api_endpoint_;
    cmd += " -H \"Content-Type: application/json\"";
    cmd += " -H \"Authorization: Bearer " + api_key_ + "\"";
    cmd += " -d @\"" + temp_file + "\"";

    std::array<char, 8192> buffer;
    std::string result;
    std::unique_ptr<FILE, PipeDeleter> pipe(popen(cmd.c_str(), "r"));
    if (!pipe) {
        std::remove(temp_file.c_str());
        throw std::runtime_error("Failed to run curl command");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }

    std::remove(temp_file.c_str());
    return result;
}

std::string LLM::parse_response(const std::string& response) {
    std::size_t message_pos = response.find("\"message\":{");
    if (message_pos == std::string::npos) {
        throw std::runtime_error("Failed to parse response: message field not found");
    }

    std::size_t message_end = find_matching_brace(response, message_pos + 10);
    if (message_end == std::string::npos) {
        throw std::runtime_error("Failed to parse response: message end not found");
    }

    std::size_t content_pos = response.find("\"content\":\"", message_pos + 10);
    if (content_pos == std::string::npos || content_pos >= message_end) {
        throw std::runtime_error("Failed to parse response: content field not found");
    }

    std::size_t quote_start = content_pos + 11;

    std::size_t quote_end = quote_start;
    while (quote_end < response.size() && quote_end < message_end) {
        if (response[quote_end] == '\\' && quote_end + 1 < response.size()) {
            quote_end += 2;
        } else if (response[quote_end] == '"') {
            break;
        } else {
            ++quote_end;
        }
    }

    if (quote_end >= response.size() || quote_end >= message_end) {
        throw std::runtime_error("Failed to parse response: content value end not found");
    }

    std::string raw_content = response.substr(quote_start, quote_end - quote_start);

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
