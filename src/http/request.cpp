#include "eventcore/http/request.h"
#include <algorithm>
#include <cctype>
#include <sstream>

namespace eventcore {
    namespace http {

        std::string Request::get_header(const std::string& name) const {
            auto it = headers_.find(name);
            return it != headers_.end() ? it->second : "";
        }

        bool Request::has_header(const std::string& name) const {
            return headers_.find(name) != headers_.end();
        }

        void Request::set_header(const std::string& name, const std::string& value) {
            headers_[name] = value;
        }

        void Request::reset() {
            method_ = Method::UNKNOWN; path_.clear(); query_.clear();
            version_ = Version::UNKNOWN; headers_.clear(); body_.clear();
        }

        Method Request::string_to_method(const std::string& str) {
            if (str == "GET") return Method::GET;
            if (str == "POST") return Method::POST;
            if (str == "PUT") return Method::PUT;
            if (str == "DELETE") return Method::DELETE;
            if (str == "HEAD") return Method::HEAD;
            if (str == "OPTIONS") return Method::OPTIONS;
            if (str == "PATCH") return Method::PATCH;
            return Method::UNKNOWN;
        }

        std::string Request::method_to_string(Method method) {
            switch (method) {
                case Method::GET: return "GET";
                case Method::POST: return "POST";
                case Method::PUT: return "PUT";
                case Method::DELETE: return "DELETE";
                case Method::HEAD: return "HEAD";
                case Method::OPTIONS: return "OPTIONS";
                case Method::PATCH: return "PATCH";
                default: return "UNKNOWN";
            }
        }

        Version Request::string_to_version(const std::string& str) {
            if (str == "HTTP/1.0") return Version::HTTP_1_0;
            if (str == "HTTP/1.1") return Version::HTTP_1_1;
            if (str == "HTTP/2.0") return Version::HTTP_2_0;
            return Version::UNKNOWN;
        }

    } // namespace http
} // namespace eventcore
