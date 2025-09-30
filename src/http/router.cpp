#include "eventcore/http/router.h"
#include <algorithm>

namespace eventcore {
    namespace http {

        void Router::add_route(Method method, const std::string& pattern, Handler handler) {
            Route route; route.pattern = pattern;
            if (pattern.find('(') != std::string::npos || pattern.find('[') != std::string::npos || pattern.find('*') != std::string::npos) {
                route.regex = std::regex(pattern); route.is_regex = true;
            } else route.is_regex = false;
            route.handler = handler; routes_[method].push_back(route);
        }

        void Router::get(const std::string& pattern, Handler handler) { add_route(Method::GET, pattern, handler); }
        void Router::post(const std::string& pattern, Handler handler) { add_route(Method::POST, pattern, handler); }
        void Router::put(const std::string& pattern, Handler handler) { add_route(Method::PUT, pattern, handler); }
        void Router::del(const std::string& pattern, Handler handler) { add_route(Method::DELETE, pattern, handler); }

        void Router::use(Middleware middleware) { use("", middleware); }
        void Router::use(const std::string& prefix, Middleware middleware) { middlewares_.emplace_back(prefix, middleware); }

        void Router::set_not_found_handler(Handler handler) { not_found_handler_ = handler; }
        void Router::set_error_handler(std::function<Response(const std::exception&)> handler) { error_handler_ = handler; }

        Response Router::route(const Request& request) const {
            try {
                Request modified_request = request; Response response;
                // Apply middlewares (C++14 compatible loop)
                for (const auto& middleware_pair : middlewares_) {
                    const std::string& prefix = middleware_pair.first;
                    const Middleware& middleware = middleware_pair.second;
                    if (prefix.empty() || request.path().find(prefix) == 0) {
                        middleware(modified_request, response);
                    }
                }
                auto method_routes = routes_.find(request.method());
                if (method_routes != routes_.end()) {
                    for (const auto& route : method_routes->second) {
                        if (route.is_regex) { if (std::regex_match(request.path(), route.regex)) return route.handler(modified_request); }
                        else { if (route.pattern == request.path()) return route.handler(modified_request); }
                    }
                }
                if (not_found_handler_) return not_found_handler_(modified_request);
                return default_404();
            } catch (const std::exception& e) {
                if (error_handler_) return error_handler_(e);
                return default_error(e);
            }
        }

        Response Router::default_404() const {
            Response response; response.set_status(404, "Not Found"); response.set_content_type("text/html");
            response.set_body("<html><body><h1>404 Not Found</h1></body></html>"); return response;
        }

        Response Router::default_error(const std::exception& e) const {
            Response response; response.set_status(500, "Internal Server Error"); response.set_content_type("text/html");
            response.set_body("<html><body><h1>500 Internal Server Error</h1><p>" + std::string(e.what()) + "</p></body></html>");
            return response;
        }

    } // namespace http
} // namespace eventcore
