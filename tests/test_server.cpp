#include <gtest/gtest.h>
#include "eventcore/server/config.h"

using namespace eventcore::server;

TEST(ConfigTest, DefaultValues) {
    Config cfg;
    EXPECT_EQ(cfg.port, 8080);
    EXPECT_EQ(cfg.host, "0.0.0.0");
    EXPECT_TRUE(cfg.tcp_nodelay);
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
