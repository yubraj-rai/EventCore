#pragma once
#include "request.h"
#include "response.h"
#include <functional>
#include <unordered_map>
#include <regex>
#include <memory>

namespace eventcore {
    namespace http {

        using Handler = std::function<Response(const Request&)>;
        using Middleware = std::function<void(Request&, Response&)>;

        class Router {
            public:
                void add_route(Method method, const std::string& pattern, Handler handler);
                void get(const std::string& pattern, Handler handler);
                void post(const std::string& pattern, Handler handler);
                void put(const std::string& pattern, Handler handler);
                void del(const std::string& pattern, Handler handler);
                void use(Middleware middleware);
                void use(const std::string& prefix, Middleware middleware);
                void set_not_found_handler(Handler handler);
                void set_error_handler(std::function<Response(const std::exception&)> handler);
                Response route(const Request& request) const;

            private:
                struct Route {
                    std::string pattern;
                    std::regex regex;
                    Handler handler;
                    bool is_regex;
                };

                std::unordered_map<Method, std::vector<Route>> routes_;
                std::vector<std::pair<std::string, Middleware>> middlewares_;
                Handler not_found_handler_;
                std::function<Response(const std::exception&)> error_handler_;

                Response default_404() const;
                Response default_error(const std::exception& e) const;
        };

    } // namespace http
} // namespace eventcore
