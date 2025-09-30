#include "eventcore/http/response.h"
#include <sstream>

namespace eventcore {
    namespace http {

        Response::Response() : status_code_(200), status_message_("OK"), keep_alive_(true) {}

        void Response::set_status(int code, const std::string& message) {
            status_code_ = code;
            status_message_ = message.empty() ? default_status_message(code) : message;
        }

        void Response::set_header(const std::string& name, const std::string& value) {
            headers_[name] = value;
        }

        void Response::set_body(const std::string& body) {
            body_ = body;
            set_header("Content-Length", std::to_string(body_.size()));
        }

        void Response::append_body(const std::string& data) {
            body_ += data;
            set_header("Content-Length", std::to_string(body_.size()));
        }

        void Response::set_content_type(const std::string& type) {
            set_header("Content-Type", type);
        }

        void Response::set_keep_alive(bool keep_alive) {
            keep_alive_ = keep_alive;
            set_header("Connection", keep_alive ? "keep-alive" : "close");
        }

        std::string Response::to_string() const {
            std::stringstream ss;
            ss << "HTTP/1.1 " << status_code_ << " " << status_message_ << "\r\n";
            for (const auto& header : headers_) ss << header.first << ": " << header.second << "\r\n";
            if (headers_.find("Connection") == headers_.end()) {
                ss << "Connection: " << (keep_alive_ ? "keep-alive" : "close") << "\r\n";
            }
            if (headers_.find("Content-Length") == headers_.end() && !body_.empty()) {
                ss << "Content-Length: " << body_.size() << "\r\n";
            }
            ss << "\r\n" << body_;
            return ss.str();
        }

        Response Response::make_404() {
            Response resp; resp.set_status(404, "Not Found"); resp.set_content_type("text/html");
            resp.set_body("<html><body><h1>404 Not Found</h1></body></html>"); return resp;
        }

        Response Response::make_500() {
            Response resp; resp.set_status(500, "Internal Server Error"); resp.set_content_type("text/html");
            resp.set_body("<html><body><h1>500 Internal Server Error</h1></body></html>"); return resp;
        }

        Response Response::make_json(int code, const std::string& json) {
            Response resp; resp.set_status(code); resp.set_content_type("application/json");
            resp.set_body(json); return resp;
        }

        Response Response::make_html(int code, const std::string& html) {
            Response resp; resp.set_status(code); resp.set_content_type("text/html");
            resp.set_body(html); return resp;
        }

        std::string Response::default_status_message(int code) const {
            switch (code) {
                case 200: return "OK"; case 201: return "Created"; case 204: return "No Content";
                case 301: return "Moved Permanently"; case 302: return "Found"; case 304: return "Not Modified";
                case 400: return "Bad Request"; case 401: return "Unauthorized"; case 403: return "Forbidden";
                case 404: return "Not Found"; case 405: return "Method Not Allowed";
                case 500: return "Internal Server Error"; case 502: return "Bad Gateway"; case 503: return "Service Unavailable";
                default: return "Unknown";
            }
        }

    } // namespace http
} // namespace eventcore
