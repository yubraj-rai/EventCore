#pragma once
#include <string>
#include <unordered_map>

namespace eventcore {
    namespace http {

        class Response {
            public:
                Response();
                int status_code() const { return status_code_; }
                const std::string& status_message() const { return status_message_; }
                const std::unordered_map<std::string, std::string>& headers() const { return headers_; }
                const std::string& body() const { return body_; }

                void set_status(int code, const std::string& message = "");
                void set_header(const std::string& name, const std::string& value);
                void set_body(const std::string& body);
                void append_body(const std::string& data);
                void set_content_type(const std::string& type);
                void set_keep_alive(bool keep_alive);
                std::string to_string() const;

                static Response make_404();
                static Response make_500();
                static Response make_json(int code, const std::string& json);
                static Response make_html(int code, const std::string& html);

            private:
                int status_code_;
                std::string status_message_;
                std::unordered_map<std::string, std::string> headers_;
                std::string body_;
                bool keep_alive_ = true;
                std::string default_status_message(int code) const;
        };

    } // namespace http
} // namespace eventcore
