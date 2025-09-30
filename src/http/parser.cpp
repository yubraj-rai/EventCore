#include "eventcore/http/parser.h"
#include <sstream>
#include <algorithm>

namespace eventcore {
    namespace http {

        Parser::Parser() : state_(kExpectRequestLine), request_(nullptr), content_length_(0) {}

        bool Parser::parse_request(net::Buffer* buffer, Request* request) {
            request_ = request; bool ok = true; bool has_more = true;
            while (has_more) {
                switch (state_) {
                    case kExpectRequestLine:
                        if (const char* crlf = buffer->find_crlf()) {
                            ok = parse_request_line(buffer->peek(), crlf);
                            if (ok) { buffer->retrieve(crlf - buffer->peek() + 2); state_ = kExpectHeaders; }
                        } else has_more = false; break;
                    case kExpectHeaders:
                        if (parse_headers(buffer)) {
                            if (content_length_ > 0) state_ = kExpectBody;
                            else { state_ = kComplete; has_more = false; }
                        } else has_more = false; break;
                    case kExpectBody:
                        if (parse_body(buffer)) { state_ = kComplete; has_more = false; }
                        else has_more = false; break;
                    case kComplete: has_more = false; break;
                }
                if (!ok) break;
            }
            return ok;
        }

        void Parser::reset() { state_ = kExpectRequestLine; request_ = nullptr; content_length_ = 0; }

        bool Parser::parse_request_line(const char* begin, const char* end) {
            std::string line(begin, end); std::istringstream iss(line);
            std::string method, path, version;
            if (!(iss >> method >> path >> version)) return false;
            request_->set_method(Request::string_to_method(method));
            size_t query_pos = path.find('?');
            if (query_pos != std::string::npos) {
                request_->set_path(path.substr(0, query_pos));
                request_->set_query(path.substr(query_pos + 1));
            } else request_->set_path(path);
            request_->set_version(Request::string_to_version(version));
            return request_->method() != Method::UNKNOWN && request_->version() != Version::UNKNOWN;
        }

        bool Parser::parse_headers(net::Buffer* buffer) {
            while (const char* crlf = buffer->find_crlf()) {
                const char* colon = std::find(buffer->peek(), crlf, ':');
                if (colon != crlf) {
                    std::string name(buffer->peek(), colon); ++colon;
                    while (colon < crlf && isspace(*colon)) ++colon;
                    std::string value(colon, crlf); request_->set_header(name, value);
                    if (name == "Content-Length") content_length_ = std::stoul(value);
                } else { buffer->retrieve(crlf - buffer->peek() + 2); return true; }
                buffer->retrieve(crlf - buffer->peek() + 2);
            }
            return false;
        }

        bool Parser::parse_body(net::Buffer* buffer) {
            if (buffer->readable_bytes() >= content_length_) {
                request_->set_body(buffer->retrieve_as_string(content_length_)); return true;
            }
            return false;
        }

    } // namespace http
} // namespace eventcore
