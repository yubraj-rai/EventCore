#pragma once
#include <string>
#include <unordered_map>

namespace eventcore {
    namespace http {

        enum class Method { GET, POST, PUT, DELETE, HEAD, OPTIONS, PATCH, UNKNOWN };
        enum class Version { HTTP_1_0, HTTP_1_1, HTTP_2_0, UNKNOWN };

        class Request {
            public:
                Method method() const { return method_; }
                const std::string& path() const { return path_; }
                const std::string& query() const { return query_; }
                Version version() const { return version_; }
                const std::unordered_map<std::string, std::string>& headers() const { return headers_; }
                const std::string& body() const { return body_; }

                std::string get_header(const std::string& name) const;
                bool has_header(const std::string& name) const;
                void set_method(Method method) { method_ = method; }
                void set_path(const std::string& path) { path_ = path; }
                void set_query(const std::string& query) { query_ = query; }
                void set_version(Version version) { version_ = version; }
                void set_header(const std::string& name, const std::string& value);
                void set_body(const std::string& body) { body_ = body; }
                void reset();

                static Method string_to_method(const std::string& str);
                static std::string method_to_string(Method method);
                static Version string_to_version(const std::string& str);

            private:
                Method method_ = Method::UNKNOWN;
                std::string path_;
                std::string query_;
                Version version_ = Version::UNKNOWN;
                std::unordered_map<std::string, std::string> headers_;
                std::string body_;
        };

    } // namespace http
} // namespace eventcore
