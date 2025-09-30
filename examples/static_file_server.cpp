#include "eventcore/server/server.h"
#include "eventcore/core/logger.h"
#include <fstream>
#include <sstream>
#include <iostream>

std::string read_file(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) throw std::runtime_error("Cannot open file: " + path);
    std::stringstream buffer; buffer << file.rdbuf(); return buffer.str();
}

std::string get_content_type(const std::string& path) {
    if (path.find(".html") != std::string::npos) return "text/html";
    if (path.find(".css") != std::string::npos) return "text/css";
    if (path.find(".js") != std::string::npos) return "application/javascript";
    if (path.find(".json") != std::string::npos) return "application/json";
    if (path.find(".png") != std::string::npos) return "image/png";
    if (path.find(".jpg") != std::string::npos || path.find(".jpeg") != std::string::npos) return "image/jpeg";
    return "text/plain";
}

int main() {
    try {
        eventcore::server::Config config;
        config.port = 8080;

        eventcore::server::Server server(config);

        server.router().get(".*", [](const eventcore::http::Request& req) {
                std::string path = req.path();
                if (path == "/") path = "/index.html";
                std::string file_path = "." + path;
                if (file_path.find("..") != std::string::npos) return eventcore::http::Response::make_404();
                try {
                std::string content = read_file(file_path);
                eventcore::http::Response resp;
                resp.set_status(200);
                resp.set_content_type(get_content_type(file_path));
                resp.set_body(content);
                return resp;
                } catch (const std::exception&) {
                return eventcore::http::Response::make_404();
                }
                });

        LOG_INFO("Starting static file server on port 8080...");
        LOG_INFO("Serving files from current directory");
        server.start();

        std::cout << "Press Enter to stop the server..." << std::endl;
        std::cin.get();

        server.stop();

    } catch (const std::exception& e) {
        LOG_ERROR("Server error: ", e.what());
        return 1;
    }

    return 0;
}
