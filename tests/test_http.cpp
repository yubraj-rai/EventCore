#include <gtest/gtest.h>
#include "eventcore/http/request.h"
#include "eventcore/http/response.h"
#include "eventcore/http/router.h"

using namespace eventcore::http;

TEST(HttpRequestTest, MethodConversion) {
    EXPECT_EQ(Request::string_to_method("GET"), Method::GET);
    EXPECT_EQ(Request::string_to_method("POST"), Method::POST);
    EXPECT_EQ(Request::string_to_method("UNKNOWN"), Method::UNKNOWN);
    EXPECT_EQ(Request::method_to_string(Method::GET), "GET");
    EXPECT_EQ(Request::method_to_string(Method::POST), "POST");
}

TEST(HttpResponseTest, BasicCreation) {
    Response resp;
    resp.set_status(200, "OK");
    resp.set_body("Hello World");
    EXPECT_EQ(resp.status_code(), 200);
    EXPECT_EQ(resp.status_message(), "OK");
    EXPECT_EQ(resp.body(), "Hello World");
}

TEST(HttpRouterTest, BasicRouting) {
    Router router;
    bool handler_called = false;
    router.get("/test", [&](const Request& req) {
            handler_called = true;
            Response resp; resp.set_status(200); return resp;
            });
    Request req; req.set_method(Method::GET); req.set_path("/test");
    Response resp = router.route(req);
    EXPECT_TRUE(handler_called);
    EXPECT_EQ(resp.status_code(), 200);
}

TEST(HttpRouterTest, NotFound) {
    Router router;
    Request req; req.set_method(Method::GET); req.set_path("/nonexistent");
    Response resp = router.route(req);
    EXPECT_EQ(resp.status_code(), 404);
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
