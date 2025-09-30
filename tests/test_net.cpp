#include <gtest/gtest.h>
#include "eventcore/net/socket.h"
#include "eventcore/net/buffer.h"
#include "eventcore/net/address.h"

using namespace eventcore::net;

TEST(SocketTest, Creation) {
    auto result = Socket::create_tcp();
    EXPECT_TRUE(result.is_ok());
}

TEST(BufferTest, BasicOperations) {
    Buffer buf;
    buf.append("Hello");
    EXPECT_EQ(buf.readable_bytes(), 5);
    auto str = buf.retrieve_all_as_string();
    EXPECT_EQ(str, "Hello");
}

TEST(AddressTest, Construction) {
    Address addr("127.0.0.1", 8080);
    EXPECT_EQ(addr.ip(), "127.0.0.1");
    EXPECT_EQ(addr.port(), 8080);
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
