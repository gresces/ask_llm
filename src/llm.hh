#ifndef LLM_HH
#define LLM_HH

#include <string>
#include <vector>

namespace llm {

struct Message {
    std::string role;
    std::string content;
};

class LLM {
public:
    LLM(const std::string& api_endpoint,
        const std::string& api_key,
        const std::string& model,
        bool enable_context,
        const std::string& system_prompt);

    std::string ask(const std::string& question, const std::string& prefix = "");

private:
    std::string api_endpoint_;
    std::string api_key_;
    std::string model_;
    bool enable_context_;
    std::vector<Message> history_;
    std::string system_prompt_;

    void check_curl_available();
    std::string build_request_body(const std::string& question);
    std::string execute_curl_stream(const std::string& request_body, const std::string& prefix);
    std::string parse_stream_line(const std::string& line);
    std::string escape_json(const std::string& str);
};

} // namespace llm

#endif // LLM_HH
