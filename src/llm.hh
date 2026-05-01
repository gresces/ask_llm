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
        bool enable_context);

    std::string ask(const std::string& question);

private:
    std::string api_endpoint_;
    std::string api_key_;
    std::string model_;
    bool enable_context_;
    std::vector<Message> history_;

    void check_curl_available();
    std::string build_request_body(const std::string& question);
    std::string execute_curl(const std::string& request_body);
    std::string parse_response(const std::string& response);
    std::string escape_json(const std::string& str);
};

} // namespace llm

#endif // LLM_HH
