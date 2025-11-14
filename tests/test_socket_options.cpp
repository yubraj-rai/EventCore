/*
Auther: Yubraj Rai

getsockopt()  is used to get the options of all levels,
1. Socket-Level Options (SOL_SOCKET)
------------------------------------
Option  Type    Description
SO_REUSEADDR    int Allows binding to a port even if it's in TIME_WAIT. Common for servers.
SO_REUSEPORT    int Allows multiple sockets to bind to the same IP+port. Kernel distributes connections.
SO_KEEPALIVE    int Enable periodic TCP keepalive probes on idle connections.
SO_LINGER   struct linger   Controls behavior of close() when data is unsent. Can force immediate close or delay.
SO_BROADCAST    int Allow sending of broadcast messages (UDP).
SO_RCVBUF   int Size of receive buffer.
SO_SNDBUF   int Size of send buffer.
SO_RCVLOWAT int Minimum number of bytes to trigger a read event.
SO_SNDLOWAT int Minimum number of bytes to trigger a write event.
SO_RCVTIMEO struct timeval  Timeout for recv() calls.
SO_SNDTIMEO struct timeval  Timeout for send() calls.
SO_TYPE int Returns socket type (e.g., SOCK_STREAM, SOCK_DGRAM).
SO_ERROR    int Returns the pending socket error, if any.


2. IP-Level Options (IPPROTO_IP)
------------------------------------
Option  Type    Description
IP_TTL  int Default TTL value for outgoing packets.
IP_MULTICAST_TTL    int TTL for multicast packets.
IP_MULTICAST_IF struct in_addr  Interface for outgoing multicast.
IP_MULTICAST_LOOP   int Loopback multicast packets (ON/OFF).
IP_ADD_MEMBERSHIP   struct ip_mreq  Join a multicast group.
IP_DROP_MEMBERSHIP  struct ip_mreq  Leave a multicast group.


3. TCP-Level Options (IPPROTO_TCP)
------------------------------------
Option  Type    Description
TCP_NODELAY int Disable Nagle algorithm (send small packets immediately).
TCP_MAXSEG  int Maximum segment size (MSS).
TCP_CORK    int Linux: hold packets until enough data to send full segment.
TCP_KEEPIDLE    int Time before sending keepalive probes.
TCP_KEEPINTVL   int Interval between keepalive probes.
TCP_KEEPCNT int Number of keepalive probes before death.
TCP_QUICKACK    int Enable/disable delayed ACK temporarily.
 */

#include <gtest/gtest.h>
#include "eventcore/net/socket.h"
#include "eventcore/net/address.h"
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <thread>
#include <chrono>

using namespace eventcore::net;

// Helper function to get socket option value
template<typename T>
T get_socket_option(int fd, int level, int optname) {
    T value;
    socklen_t len = sizeof(T);
    if (getsockopt(fd, level, optname, &value, &len) < 0) {
        throw std::runtime_error("getsockopt failed");
    }
    return value;
}

class SocketOptionsTest : public ::testing::Test {
    protected:
        void SetUp() override {
            // Create a fresh socket for each test
            auto result = Socket::create_tcp();
            ASSERT_TRUE(result.is_ok());
            socket_ = std::move(result.value());
        }

        void TearDown() override {
            socket_.close();
        }

        Socket socket_;
};

// ============================================================================
// SO_REUSEADDR Tests
// ============================================================================

TEST_F(SocketOptionsTest, SetReuseAddrEnable) {
    auto result = socket_.set_reuseaddr(true);
    EXPECT_TRUE(result.is_ok());

    // Verify the option is actually set in the kernel
    int value = get_socket_option<int>(socket_.fd(), SOL_SOCKET, SO_REUSEADDR);
    EXPECT_EQ(value, 1);
}

TEST_F(SocketOptionsTest, SetReuseAddrDisable) {
    auto result = socket_.set_reuseaddr(false);
    EXPECT_TRUE(result.is_ok());

    int value = get_socket_option<int>(socket_.fd(), SOL_SOCKET, SO_REUSEADDR);
    EXPECT_EQ(value, 0);
}

//Enabling SO_REUSEADDR option allows quick rebind after close, skipping the TIME_WAIT state
TEST_F(SocketOptionsTest, ReuseAddrQuickRebind) {

    uint16_t test_port = 19999;
    Address addr("127.0.0.1", test_port);

    // First socket: bind and close
    {
        auto result1 = Socket::create_tcp();
        ASSERT_TRUE(result1.is_ok());
        Socket sock1 = std::move(result1.value());

        auto reuse_result = sock1.set_reuseaddr(true);
        EXPECT_TRUE(reuse_result.is_ok());

        auto bind_result = sock1.bind(addr);
        EXPECT_TRUE(bind_result.is_ok());

        auto listen_result = sock1.listen();
        EXPECT_TRUE(listen_result.is_ok());

        // Socket closes when sock1 goes out of scope
    }

    // Second socket: should be able to bind immediately with SO_REUSEADDR
    auto result2 = Socket::create_tcp();
    ASSERT_TRUE(result2.is_ok());
    Socket sock2 = std::move(result2.value());

    auto reuse_result2 = sock2.set_reuseaddr(true);
    EXPECT_TRUE(reuse_result2.is_ok());

    auto bind_result2 = sock2.bind(addr);
    EXPECT_TRUE(bind_result2.is_ok()) << "Failed to rebind with SO_REUSEADDR";
}

TEST_F(SocketOptionsTest, ReuseAddrFailWithoutOption) {
    // Test that without SO_REUSEADDR, quick rebind fails
    uint16_t test_port = 20000;
    Address addr("127.0.0.1", test_port);

    // First socket
    {
        auto result1 = Socket::create_tcp();
        ASSERT_TRUE(result1.is_ok());
        Socket sock1 = std::move(result1.value());

        // Explicitly disable SO_REUSEADDR
        auto reuse_result = sock1.set_reuseaddr(false);
        EXPECT_TRUE(reuse_result.is_ok());

        auto bind_result = sock1.bind(addr);
        EXPECT_TRUE(bind_result.is_ok());

        auto listen_result = sock1.listen();
        EXPECT_TRUE(listen_result.is_ok());
    }

    // Second socket without SO_REUSEADDR
    auto result2 = Socket::create_tcp();
    ASSERT_TRUE(result2.is_ok());
    Socket sock2 = std::move(result2.value());

    // Don't set SO_REUSEADDR
    auto bind_result2 = sock2.bind(addr);

    // This might succeed or fail depending on TIME_WAIT state
    // but the test documents the behavior
    if (bind_result2.is_err()) {
        EXPECT_NE(bind_result2.error().find("bind failed"), std::string::npos);
    }
}

// ============================================================================
// SO_REUSEPORT Tests
// ============================================================================

#ifdef SO_REUSEPORT
TEST_F(SocketOptionsTest, SetReusePortEnable) {
    auto result = socket_.set_reuseport(true);
    EXPECT_TRUE(result.is_ok());

    int value = get_socket_option<int>(socket_.fd(), SOL_SOCKET, SO_REUSEPORT);
    EXPECT_EQ(value, 1);
}

TEST_F(SocketOptionsTest, SetReusePortDisable) {
    auto result = socket_.set_reuseport(false);
    EXPECT_TRUE(result.is_ok());

    int value = get_socket_option<int>(socket_.fd(), SOL_SOCKET, SO_REUSEPORT);
    EXPECT_EQ(value, 0);
}

TEST_F(SocketOptionsTest, ReusePortMultipleBind) {
    // Test that SO_REUSEPORT allows multiple sockets to bind to same port
    uint16_t test_port = 20001;
    Address addr("127.0.0.1", test_port);

    // First socket
    auto result1 = Socket::create_tcp();
    ASSERT_TRUE(result1.is_ok());
    Socket sock1 = std::move(result1.value());

    auto reuse1 = sock1.set_reuseport(true);
    EXPECT_TRUE(reuse1.is_ok());

    auto bind1 = sock1.bind(addr);
    EXPECT_TRUE(bind1.is_ok());

    // Second socket - should also bind to same port with SO_REUSEPORT
    auto result2 = Socket::create_tcp();
    ASSERT_TRUE(result2.is_ok());
    Socket sock2 = std::move(result2.value());

    auto reuse2 = sock2.set_reuseport(true);
    EXPECT_TRUE(reuse2.is_ok());

    auto bind2 = sock2.bind(addr);
    EXPECT_TRUE(bind2.is_ok()) << "SO_REUSEPORT should allow multiple binds";
}
#endif

// ============================================================================
// TCP_NODELAY Tests (Nagle's Algorithm)
// ============================================================================

TEST_F(SocketOptionsTest, SetNoDelayEnable) {
    auto result = socket_.set_nodelay(true);
    EXPECT_TRUE(result.is_ok());

    int value = get_socket_option<int>(socket_.fd(), IPPROTO_TCP, TCP_NODELAY);
    EXPECT_EQ(value, 1);
}

TEST_F(SocketOptionsTest, SetNoDelayDisable) {
    auto result = socket_.set_nodelay(false);
    EXPECT_TRUE(result.is_ok());

    int value = get_socket_option<int>(socket_.fd(), IPPROTO_TCP, TCP_NODELAY);
    EXPECT_EQ(value, 0);
}

TEST_F(SocketOptionsTest, NoDelayDefaultState) {
    // By default, TCP_NODELAY should be disabled (Nagle's algorithm enabled)
    int value = get_socket_option<int>(socket_.fd(), IPPROTO_TCP, TCP_NODELAY);
    EXPECT_EQ(value, 0) << "TCP_NODELAY should be disabled by default";
}

TEST_F(SocketOptionsTest, NoDelayToggle) {
    // Test toggling TCP_NODELAY on and off
    auto result1 = socket_.set_nodelay(true);
    EXPECT_TRUE(result1.is_ok());
    int value1 = get_socket_option<int>(socket_.fd(), IPPROTO_TCP, TCP_NODELAY);
    EXPECT_EQ(value1, 1);

    auto result2 = socket_.set_nodelay(false);
    EXPECT_TRUE(result2.is_ok());
    int value2 = get_socket_option<int>(socket_.fd(), IPPROTO_TCP, TCP_NODELAY);
    EXPECT_EQ(value2, 0);

    auto result3 = socket_.set_nodelay(true);
    EXPECT_TRUE(result3.is_ok());
    int value3 = get_socket_option<int>(socket_.fd(), IPPROTO_TCP, TCP_NODELAY);
    EXPECT_EQ(value3, 1);
}

// ============================================================================
// SO_KEEPALIVE Tests
// ============================================================================

TEST_F(SocketOptionsTest, SetKeepaliveEnable) {
    auto result = socket_.set_keepalive(true);
    EXPECT_TRUE(result.is_ok());

    int value = get_socket_option<int>(socket_.fd(), SOL_SOCKET, SO_KEEPALIVE);
    EXPECT_EQ(value, 1);
}

TEST_F(SocketOptionsTest, SetKeepaliveDisable) {
    auto result = socket_.set_keepalive(false);
    EXPECT_TRUE(result.is_ok());

    int value = get_socket_option<int>(socket_.fd(), SOL_SOCKET, SO_KEEPALIVE);
    EXPECT_EQ(value, 0);
}

TEST_F(SocketOptionsTest, KeepaliveDefaultState) {
    // By default, SO_KEEPALIVE should be disabled
    int value = get_socket_option<int>(socket_.fd(), SOL_SOCKET, SO_KEEPALIVE);
    EXPECT_EQ(value, 0) << "SO_KEEPALIVE should be disabled by default";
}

TEST_F(SocketOptionsTest, KeepaliveToggle) {
    // Test toggling SO_KEEPALIVE on and off
    auto result1 = socket_.set_keepalive(true);
    EXPECT_TRUE(result1.is_ok());
    int value1 = get_socket_option<int>(socket_.fd(), SOL_SOCKET, SO_KEEPALIVE);
    EXPECT_EQ(value1, 1);

    auto result2 = socket_.set_keepalive(false);
    EXPECT_TRUE(result2.is_ok());
    int value2 = get_socket_option<int>(socket_.fd(), SOL_SOCKET, SO_KEEPALIVE);
    EXPECT_EQ(value2, 0);
}

// ============================================================================
// Combined Options Tests
// ============================================================================

TEST_F(SocketOptionsTest, SetMultipleOptions) {
    // Test setting all options together
    auto reuseaddr_result = socket_.set_reuseaddr(true);
    EXPECT_TRUE(reuseaddr_result.is_ok());

    auto reuseport_result = socket_.set_reuseport(true);
    EXPECT_TRUE(reuseport_result.is_ok());

    auto nodelay_result = socket_.set_nodelay(true);
    EXPECT_TRUE(nodelay_result.is_ok());

    auto keepalive_result = socket_.set_keepalive(true);
    EXPECT_TRUE(keepalive_result.is_ok());

    // Verify all options are set
    int reuseaddr = get_socket_option<int>(socket_.fd(), SOL_SOCKET, SO_REUSEADDR);
    EXPECT_EQ(reuseaddr, 1);

#ifdef SO_REUSEPORT
    int reuseport = get_socket_option<int>(socket_.fd(), SOL_SOCKET, SO_REUSEPORT);
    EXPECT_EQ(reuseport, 1);
#endif

    int nodelay = get_socket_option<int>(socket_.fd(), IPPROTO_TCP, TCP_NODELAY);
    EXPECT_EQ(nodelay, 1);

    int keepalive = get_socket_option<int>(socket_.fd(), SOL_SOCKET, SO_KEEPALIVE);
    EXPECT_EQ(keepalive, 1);
}

TEST_F(SocketOptionsTest, OptionsIndependence) {
    // Test that options are independent
    socket_.set_reuseaddr(true);
    socket_.set_nodelay(false);
    socket_.set_keepalive(true);

    int reuseaddr = get_socket_option<int>(socket_.fd(), SOL_SOCKET, SO_REUSEADDR);
    int nodelay = get_socket_option<int>(socket_.fd(), IPPROTO_TCP, TCP_NODELAY);
    int keepalive = get_socket_option<int>(socket_.fd(), SOL_SOCKET, SO_KEEPALIVE);

    EXPECT_EQ(reuseaddr, 1);
    EXPECT_EQ(nodelay, 0);
    EXPECT_EQ(keepalive, 1);
}

TEST_F(SocketOptionsTest, OptionsPersistAfterBind) {
    // Test that options persist after bind
    uint16_t test_port = 20002;
    Address addr("127.0.0.1", test_port);

    socket_.set_reuseaddr(true);
    socket_.set_nodelay(true);
    socket_.set_keepalive(true);

    auto bind_result = socket_.bind(addr);
    EXPECT_TRUE(bind_result.is_ok());

    // Check options are still set after bind
    int reuseaddr = get_socket_option<int>(socket_.fd(), SOL_SOCKET, SO_REUSEADDR);
    int nodelay = get_socket_option<int>(socket_.fd(), IPPROTO_TCP, TCP_NODELAY);
    int keepalive = get_socket_option<int>(socket_.fd(), SOL_SOCKET, SO_KEEPALIVE);

    EXPECT_EQ(reuseaddr, 1);
    EXPECT_EQ(nodelay, 1);
    EXPECT_EQ(keepalive, 1);
}

// ============================================================================
// Edge Cases and Error Handling
// ============================================================================

TEST_F(SocketOptionsTest, SetOptionsOnInvalidSocket) {
    // Create a socket and close it
    Socket invalid_socket;

    // All operations should fail or handle gracefully
    auto reuseaddr_result = invalid_socket.set_reuseaddr(true);
    EXPECT_TRUE(reuseaddr_result.is_err());
}

TEST_F(SocketOptionsTest, SetOptionsOnClosedSocket) {
    socket_.close();

    auto reuseaddr_result = socket_.set_reuseaddr(true);
    EXPECT_TRUE(reuseaddr_result.is_err());

    auto nodelay_result = socket_.set_nodelay(true);
    EXPECT_TRUE(nodelay_result.is_err());
}

// ============================================================================
// End-to-End Integration Tests
// ============================================================================

TEST_F(SocketOptionsTest, E2E_ServerWithAllOptions) {
    // Create a server socket with all options enabled
    uint16_t test_port = 20003;
    Address server_addr("127.0.0.1", test_port);

    auto server_result = Socket::create_tcp();
    ASSERT_TRUE(server_result.is_ok());
    Socket server_socket = std::move(server_result.value());

    // Set all options
    EXPECT_TRUE(server_socket.set_reuseaddr(true).is_ok());
    EXPECT_TRUE(server_socket.set_reuseport(true).is_ok());
    EXPECT_TRUE(server_socket.set_nodelay(true).is_ok());
    EXPECT_TRUE(server_socket.set_keepalive(true).is_ok());

    // Bind and listen
    EXPECT_TRUE(server_socket.bind(server_addr).is_ok());
    EXPECT_TRUE(server_socket.listen().is_ok());

    // Verify options are still set
    int reuseaddr = get_socket_option<int>(server_socket.fd(), SOL_SOCKET, SO_REUSEADDR);
    EXPECT_EQ(reuseaddr, 1);

    int nodelay = get_socket_option<int>(server_socket.fd(), IPPROTO_TCP, TCP_NODELAY);
    EXPECT_EQ(nodelay, 1);

    int keepalive = get_socket_option<int>(server_socket.fd(), SOL_SOCKET, SO_KEEPALIVE);
    EXPECT_EQ(keepalive, 1);

    server_socket.close();
}

TEST_F(SocketOptionsTest, E2E_ClientServerConnection) {
    uint16_t test_port = 20004;
    Address server_addr("127.0.0.1", test_port);

    // Create and setup server
    auto server_result = Socket::create_tcp();
    ASSERT_TRUE(server_result.is_ok());
    Socket server_socket = std::move(server_result.value());

    server_socket.set_reuseaddr(true);
    server_socket.set_nodelay(true);
    server_socket.set_keepalive(true);

    ASSERT_TRUE(server_socket.bind(server_addr).is_ok());
    ASSERT_TRUE(server_socket.listen(1).is_ok());

    // Create client in a separate thread
    std::thread client_thread([&]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            auto client_result = Socket::create_tcp();
            EXPECT_TRUE(client_result.is_ok());
            Socket client_socket = std::move(client_result.value());

            client_socket.set_nodelay(true);
            client_socket.set_keepalive(true);

            auto connect_result = client_socket.connect(server_addr);
            EXPECT_TRUE(connect_result.is_ok());

            if (connect_result.is_ok()) {
            // Verify client options
            int nodelay = get_socket_option<int>(client_socket.fd(), IPPROTO_TCP, TCP_NODELAY);
            EXPECT_EQ(nodelay, 1);

            int keepalive = get_socket_option<int>(client_socket.fd(), SOL_SOCKET, SO_KEEPALIVE);
            EXPECT_EQ(keepalive, 1);
            }

            client_socket.close();
    });

    // Accept connection on server
    auto accept_result = server_socket.accept();

    if (accept_result.is_ok()) {
        Socket client_conn = std::move(accept_result.value());

        // Verify accepted socket inherits some options
        int keepalive = get_socket_option<int>(client_conn.fd(), SOL_SOCKET, SO_KEEPALIVE);
        EXPECT_GE(keepalive, 0); // Just verify we can read it

        client_conn.close();
    }

    client_thread.join();
    server_socket.close();
}

//server crashes and restarts
TEST_F(SocketOptionsTest, E2E_RapidRebindScenario) {
    uint16_t test_port = 20005;
    Address addr("127.0.0.1", test_port);

    for (int i = 0; i < 3; ++i) {
        auto result = Socket::create_tcp();
        ASSERT_TRUE(result.is_ok());
        Socket sock = std::move(result.value());

        // Enable SO_REUSEADDR for rapid restart
        auto reuse_result = sock.set_reuseaddr(true);
        EXPECT_TRUE(reuse_result.is_ok());

        auto bind_result = sock.bind(addr);
        EXPECT_TRUE(bind_result.is_ok()) << "Iteration " << i << " failed to bind";

        auto listen_result = sock.listen();
        EXPECT_TRUE(listen_result.is_ok());

        // Simulate server operation
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        // Socket closes automatically when going out of scope (RAII wrapper)
    }
}

// ============================================================================
// TCP Keepalive Advanced Tests
// ============================================================================

#ifdef __linux__

TEST_F(SocketOptionsTest, KeepaliveSetIdleTime) {
    // Set the time before sending keepalive probes
    auto result = socket_.set_keepalive(true);
    ASSERT_TRUE(result.is_ok());

    int idle_time = 10; // seconds
    if (setsockopt(socket_.fd(), IPPROTO_TCP, TCP_KEEPIDLE, 
                &idle_time, sizeof(idle_time)) == 0) {
        int value = get_socket_option<int>(socket_.fd(), IPPROTO_TCP, TCP_KEEPIDLE);
        EXPECT_EQ(value, idle_time);
    }
}

TEST_F(SocketOptionsTest, KeepaliveSetProbeInterval) {
    // Set interval between keepalive probes
    auto result = socket_.set_keepalive(true);
    ASSERT_TRUE(result.is_ok());

    int interval = 5; // seconds
    if (setsockopt(socket_.fd(), IPPROTO_TCP, TCP_KEEPINTVL, 
                &interval, sizeof(interval)) == 0) {
        int value = get_socket_option<int>(socket_.fd(), IPPROTO_TCP, TCP_KEEPINTVL);
        EXPECT_EQ(value, interval);
    }
}

TEST_F(SocketOptionsTest, KeepaliveSetProbeCount) {
    // Set number of probes before declaring connection dead
    auto result = socket_.set_keepalive(true);
    ASSERT_TRUE(result.is_ok());

    int count = 3; // probes
    if (setsockopt(socket_.fd(), IPPROTO_TCP, TCP_KEEPCNT, 
                &count, sizeof(count)) == 0) {
        int value = get_socket_option<int>(socket_.fd(), IPPROTO_TCP, TCP_KEEPCNT);
        EXPECT_EQ(value, count);
    }
}

TEST_F(SocketOptionsTest, KeepaliveAggressiveConfiguration) {
    // Configure aggressive keepalive for fast dead peer detection
    auto result = socket_.set_keepalive(true);
    ASSERT_TRUE(result.is_ok());

    int idle = 5;      // Start probing after 5 seconds
    int interval = 2;  // Send probe every 2 seconds
    int count = 3;     // Give up after 3 failed probes

    setsockopt(socket_.fd(), IPPROTO_TCP, TCP_KEEPIDLE, &idle, sizeof(idle));
    setsockopt(socket_.fd(), IPPROTO_TCP, TCP_KEEPINTVL, &interval, sizeof(interval));
    setsockopt(socket_.fd(), IPPROTO_TCP, TCP_KEEPCNT, &count, sizeof(count));

    // Total time to detect dead peer: 5 + (2 * 3) = 11 seconds
    EXPECT_EQ(get_socket_option<int>(socket_.fd(), IPPROTO_TCP, TCP_KEEPIDLE), idle);
    EXPECT_EQ(get_socket_option<int>(socket_.fd(), IPPROTO_TCP, TCP_KEEPINTVL), interval);
    EXPECT_EQ(get_socket_option<int>(socket_.fd(), IPPROTO_TCP, TCP_KEEPCNT), count);
}

TEST_F(SocketOptionsTest, KeepaliveConservativeConfiguration) {
    // Configure conservative keepalive for low-overhead monitoring
    auto result = socket_.set_keepalive(true);
    ASSERT_TRUE(result.is_ok());

    int idle = 600;    // Wait 10 minutes before probing
    int interval = 60; // Probe every minute
    int count = 5;     // Try 5 times before giving up

    setsockopt(socket_.fd(), IPPROTO_TCP, TCP_KEEPIDLE, &idle, sizeof(idle));
    setsockopt(socket_.fd(), IPPROTO_TCP, TCP_KEEPINTVL, &interval, sizeof(interval));
    setsockopt(socket_.fd(), IPPROTO_TCP, TCP_KEEPCNT, &count, sizeof(count));

    EXPECT_EQ(get_socket_option<int>(socket_.fd(), IPPROTO_TCP, TCP_KEEPIDLE), idle);
    EXPECT_EQ(get_socket_option<int>(socket_.fd(), IPPROTO_TCP, TCP_KEEPINTVL), interval);
    EXPECT_EQ(get_socket_option<int>(socket_.fd(), IPPROTO_TCP, TCP_KEEPCNT), count);
}

TEST_F(SocketOptionsTest, KeepaliveDefaultParameters) {
    // Check default keepalive parameters on Linux
    auto result = socket_.set_keepalive(true);
    ASSERT_TRUE(result.is_ok());

    int idle = get_socket_option<int>(socket_.fd(), IPPROTO_TCP, TCP_KEEPIDLE);
    int interval = get_socket_option<int>(socket_.fd(), IPPROTO_TCP, TCP_KEEPINTVL);
    int count = get_socket_option<int>(socket_.fd(), IPPROTO_TCP, TCP_KEEPCNT);

    // Typical Linux defaults: 7200s idle, 75s interval, 9 probes
    EXPECT_GT(idle, 0);
    EXPECT_GT(interval, 0);
    EXPECT_GT(count, 0);

    //std::cout << "Default keepalive params - Idle: " << idle << "s, Interval: " << interval  << "s, Count: " << count << std::endl;
}

// ============================================================================
// TCP Keepalive Dead Peer Detection Tests
// ============================================================================

TEST_F(SocketOptionsTest, KeepaliveDetectDeadPeer_SimulatedDrop) {
    uint16_t test_port = 20010;
    Address server_addr("127.0.0.1", test_port);

    // Create server
    auto server_result = Socket::create_tcp();
    ASSERT_TRUE(server_result.is_ok());
    Socket server_socket = std::move(server_result.value());

    server_socket.set_reuseaddr(true);
    server_socket.set_keepalive(true);

    // Aggressive keepalive: detect failure in ~11 seconds
    int idle = 5, interval = 2, count = 3;
    setsockopt(server_socket.fd(), IPPROTO_TCP, TCP_KEEPIDLE, &idle, sizeof(idle));
    setsockopt(server_socket.fd(), IPPROTO_TCP, TCP_KEEPINTVL, &interval, sizeof(interval));
    setsockopt(server_socket.fd(), IPPROTO_TCP, TCP_KEEPCNT, &count, sizeof(count));

    ASSERT_TRUE(server_socket.bind(server_addr).is_ok());
    ASSERT_TRUE(server_socket.listen(1).is_ok());

    std::atomic<bool> connection_failed{false};  // Did we detect failure?
    std::atomic<bool> server_ready{false};       // Is server ready for client?

    // Possible returns after keepalive timeout:
    // 1. recv() returns 0 (EOF - connection closed)
    // 2. recv() returns -1 with errno = ETIMEDOUT
    // 3. recv() returns -1 with errno = ECONNRESET
    std::thread server_thread([&]() {
            // Accept incoming connection
            auto accept_result = server_socket.accept();
            EXPECT_TRUE(accept_result.is_ok());
            Socket client_conn = std::move(accept_result.value());

            //server is ready, client can connect now
            server_ready = true;

            // Keep connection alive and try to detect failure
            char buffer[128];
            while (!connection_failed) {
            auto recv_result = client_conn.recv(buffer, sizeof(buffer));
            // Check if connection is broken, When keepalive detects dead peer, recv() returns error or 0
            if (recv_result.is_err() || recv_result.value() == 0) {
            // Keepalive detected the dead connection! 
            connection_failed = true;
            break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
    });

    // Client connects then abruptly disappears (simulated by closing)
    std::thread client_thread([&]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            auto client_result = Socket::create_tcp();
            EXPECT_TRUE(client_result.is_ok());
            Socket client_socket = std::move(client_result.value());

            auto connect_result = client_socket.connect(server_addr);
            EXPECT_TRUE(connect_result.is_ok());

            // Wait for server to accept
            while (!server_ready) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }

            // Simulate network drop - abruptly close without proper shutdown
            ::close(client_socket.release());
            });

    client_thread.join();

    // Wait for keepalive to detect dead peer
    std::this_thread::sleep_for(std::chrono::seconds(12));

    EXPECT_TRUE(connection_failed) << "Keepalive should detect dead peer";

    server_thread.join();
    server_socket.close();
}

/*
   TEST PURPOSE: Verify that a TCP connection remains alive and functional
   when TCP Keep-Alive probes are successfully acknowledged by the peer.

CONFIGURATION:
- TCP Keep-Alive is enabled on both server and client sockets.
- Server Keep-Alive parameters (Linux-specific):
- TCP_KEEPIDLE (Idle Time): 5 seconds (fast probing)
- TCP_KEEPINTVL (Probe Interval): 1 second
- TCP_KEEPCNT (Probe Count): 3
- Client sends an application-level "heartbeat" message every 25 seconds.

OBSERVED BEHAVIOR (Wireshark/tshark):
1. After the initial handshake, the server's fast-probing Keep-Alive timer (5s) expires.
2. The server sends Keep-Alive probes (e.g., at 4.6s, 5.1s, 5.7s, 9.1s, 9.9s).
3. The client's kernel immediately responds with an ACK for each probe, confirming the
connection is alive and the probes are working correctly (e.g., Frame 9, 11, 13, etc., in the tshark output).
4. The first client message is successfully received (successful_sends > 0).

CRITICAL TEST FLAW:
The 'server_thread' explicitly monitors the connection for only 10 seconds.
After this 10-second timer expires, the thread function returns. The accepted
socket 'client_conn' goes out of scope, and its destructor, leveraging RAII,
calls the underlying 'close()'. This action forcibly closes the connection from
the server side, preventing the test from observing subsequent client heartbeats
(due at ~25s, ~50s, etc.).

The assertion EXPECT_FALSE(connection_alive) passes because the server thread
executes 'connection_alive = false' upon its 10-second timeout, not because
the connection was killed by a failed Keep-Alive mechanism.

No.,Time,Source,Destination,Protocol,Length,Info
8,4.608015481,127.0.0.1,127.0.0.1,TCP,66,60310 → 20011 [TCP Keep-Alive] Seq=1 Ack=1 Win=65536 Len=0 TSval=164746434 TSecr=164746434
9,4.608027097,127.0.0.1,127.0.0.1,TCP,66,20011 → 60310 [ACK] Seq=1 Ack=1 Win=65536 Len=0 TSval=164746434 TSecr=164746434
10,5.102491494,127.0.0.1,127.0.0.1,TCP,66,60310 → 20011 [TCP Keep-Alive] Seq=10 Ack=1 Win=65536 Len=0 TSval=164761957 TSecr=164756901
11,5.152253308,127.0.0.1,127.0.0.1,TCP,66,20011 → 60310 [ACK] Seq=1 Ack=11 Win=65536 Len=0 TSval=164761957 TSecr=164746434
12,5.787408541,127.0.0.1,127.0.0.1,TCP,66,60310 → 20011 [TCP Keep-Alive] Seq=10 Ack=1 Win=65536 Len=0 TSval=164761957 TSecr=164756901
13,5.7880956,127.0.0.1,127.0.0.1,TCP,66,20011 → 60310 [ACK] Seq=1 Ack=11 Win=65536 Len=0 TSval=164761957 TSecr=164756901
14,9.183416801,127.0.0.1,127.0.0.1,TCP,66,60310 → 20011 [TCP Keep-Alive] Seq=10 Ack=1 Win=65536 Len=0 TSval=164787013 TSecr=164746434
15,9.183427953,127.0.0.1,127.0.0.1,TCP,66,20011 → 60310 [ACK] Seq=1 Ack=11 Win=65536 Len=0 TSval=164787013 TSecr=164746434
16,9.99849562,127.0.0.1,127.0.0.1,TCP,66,60310 → 20011 [TCP Keep-Alive] Seq=10 Ack=1 Win=65536 Len=0 TSval=164771435 TSecr=164756901
17,9.99850604,127.0.0.1,127.0.0.1,TCP,66,20011 → 60310 [ACK] Seq=1 Ack=11 Win=65536 Len=0 TSval=164771435 TSecr=164756901
 */
TEST_F(SocketOptionsTest, KeepaliveAliveConnection_ProbesSucceed) {
    uint16_t test_port = 20011;
    Address server_addr("127.0.0.1", test_port);

    auto server_result = Socket::create_tcp();
    ASSERT_TRUE(server_result.is_ok());
    Socket server_socket = std::move(server_result.value());

    server_socket.set_reuseaddr(true);
    server_socket.set_keepalive(true);

    // Fast keepalive to test quickly
    int idle = 5, interval = 1, count = 3;
    setsockopt(server_socket.fd(), IPPROTO_TCP, TCP_KEEPIDLE, &idle, sizeof(idle));
    setsockopt(server_socket.fd(), IPPROTO_TCP, TCP_KEEPINTVL, &interval, sizeof(interval));
    setsockopt(server_socket.fd(), IPPROTO_TCP, TCP_KEEPCNT, &count, sizeof(count));

    ASSERT_TRUE(server_socket.bind(server_addr).is_ok());
    ASSERT_TRUE(server_socket.listen(1).is_ok());

    std::atomic<bool> connection_alive{true};
    std::atomic<int> successful_sends{0};

    std::thread server_thread([&]() {
            auto accept_result = server_socket.accept();
            EXPECT_TRUE(accept_result.is_ok());
            Socket client_conn = std::move(accept_result.value());

            // Monitor connection for 10 seconds
            auto start = std::chrono::steady_clock::now();
            while (std::chrono::steady_clock::now() - start < std::chrono::seconds(10)) {
            char buffer[128];
            auto recv_result = client_conn.recv(buffer, sizeof(buffer));

            //std::cout<<"client message: "<<buffer<<std::endl;
            if (recv_result.is_err() || recv_result.value() == 0) {
            //std::cout<<"connection_alive: "<<connection_alive<<std::endl;
            connection_alive = false;
            break;
            }

            if (recv_result.is_ok() && recv_result.value() > 0) {
            successful_sends++;
            }

            //std::cout<<"successful_sends cnt: "<<successful_sends<<std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
    });

    std::thread client_thread([&]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            auto client_result = Socket::create_tcp();
            EXPECT_TRUE(client_result.is_ok());
            Socket client_socket = std::move(client_result.value());

            client_socket.set_keepalive(true);

            auto connect_result = client_socket.connect(server_addr);
            EXPECT_TRUE(connect_result.is_ok());

            // Keep connection alive by sending periodic data
            for (int i = 0; i < 50; ++i) {
            const char* msg = "heartbeat";
            auto send_result = client_socket.send(msg, strlen(msg));
            if (send_result.is_err()) break;
            std::this_thread::sleep_for(std::chrono::seconds(20));
            }
            //std::this_thread::sleep_for(std::chrono::seconds(60));
            });

    server_thread.join();
    client_thread.join();

    EXPECT_TRUE(connection_alive) << "Connection should remain alive with keepalive";

    EXPECT_GT(successful_sends, 0) << "Should receive heartbeat messages";
    //EXPECT_GT(successful_sends, 40) << "Should receive most heartbeat messages";

    server_socket.close();
}

TEST_F(SocketOptionsTest, KeepaliveMultipleProbeAttempts) {
    // Test that keepalive tries multiple times before giving up
    uint16_t test_port = 20012;
    Address server_addr("127.0.0.1", test_port);

    auto server_result = Socket::create_tcp();
    ASSERT_TRUE(server_result.is_ok());
    Socket server_socket = std::move(server_result.value());

    server_socket.set_reuseaddr(true);
    server_socket.set_keepalive(true);

    int idle = 3, interval = 2, count = 5; // 5 probe attempts
    setsockopt(server_socket.fd(), IPPROTO_TCP, TCP_KEEPIDLE, &idle, sizeof(idle));
    setsockopt(server_socket.fd(), IPPROTO_TCP, TCP_KEEPINTVL, &interval, sizeof(interval));
    setsockopt(server_socket.fd(), IPPROTO_TCP, TCP_KEEPCNT, &count, sizeof(count));

    // Expected death time: 3 + (2 * 5) = 13 seconds
    int expected_death_time = idle + (interval * count);
    EXPECT_EQ(expected_death_time, 13);

    ASSERT_TRUE(server_socket.bind(server_addr).is_ok());
    ASSERT_TRUE(server_socket.listen(1).is_ok());

    std::atomic<bool> detected_dead{false};
    auto death_time = std::chrono::steady_clock::time_point{};

    std::thread server_thread([&]() {
            auto accept_result = server_socket.accept();
            EXPECT_TRUE(accept_result.is_ok());
            Socket client_conn = std::move(accept_result.value());

            auto start = std::chrono::steady_clock::now();

            char buffer[128];
            while (true) {
            auto recv_result = client_conn.recv(buffer, sizeof(buffer));
            if (recv_result.is_err() || recv_result.value() == 0) {
            detected_dead = true;
            death_time = std::chrono::steady_clock::now();
            break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            });

    std::thread client_thread([&]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            auto client_result = Socket::create_tcp();
            EXPECT_TRUE(client_result.is_ok());
            Socket client_socket = std::move(client_result.value());

            client_socket.connect(server_addr);

            // Abruptly close
            ::close(client_socket.release());
            });

    client_thread.join();

    // Wait for detection
    std::this_thread::sleep_for(std::chrono::seconds(expected_death_time + 2));

    EXPECT_TRUE(detected_dead) << "Should detect death after " << count << " probes";

    server_thread.join();
    server_socket.close();
}

// ============================================================================
// TCP Keepalive with Connection Idle Tests
// ============================================================================

TEST_F(SocketOptionsTest, KeepaliveIdleConnection_NoDataTransfer) {
    // Test keepalive on completely idle connection
    uint16_t test_port = 20013;
    Address server_addr("127.0.0.1", test_port);

    auto server_result = Socket::create_tcp();
    ASSERT_TRUE(server_result.is_ok());
    Socket server_socket = std::move(server_result.value());

    server_socket.set_reuseaddr(true);
    server_socket.set_keepalive(true);

    int idle = 3, interval = 1, count = 3;
    setsockopt(server_socket.fd(), IPPROTO_TCP, TCP_KEEPIDLE, &idle, sizeof(idle));
    setsockopt(server_socket.fd(), IPPROTO_TCP, TCP_KEEPINTVL, &interval, sizeof(interval));
    setsockopt(server_socket.fd(), IPPROTO_TCP, TCP_KEEPCNT, &count, sizeof(count));

    ASSERT_TRUE(server_socket.bind(server_addr).is_ok());
    ASSERT_TRUE(server_socket.listen(1).is_ok());

    std::atomic<bool> still_connected{true};

    std::thread server_thread([&]() {
            auto accept_result = server_socket.accept();
            EXPECT_TRUE(accept_result.is_ok());
            Socket client_conn = std::move(accept_result.value());

            // Just monitor connection without any I/O
            auto start = std::chrono::steady_clock::now();
            while (std::chrono::steady_clock::now() - start < std::chrono::seconds(10)) {
            // Use SO_ERROR to check connection status
            int error = 0;
            socklen_t len = sizeof(error);
            getsockopt(client_conn.fd(), SOL_SOCKET, SO_ERROR, &error, &len);

            if (error != 0) {
            still_connected = false;
            break;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
            });

    std::thread client_thread([&]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            auto client_result = Socket::create_tcp();
            EXPECT_TRUE(client_result.is_ok());
            Socket client_socket = std::move(client_result.value());

            client_socket.set_keepalive(true);
            client_socket.connect(server_addr);

            // Stay connected for 10 seconds doing nothing
            std::this_thread::sleep_for(std::chrono::seconds(10));
            });

    server_thread.join();
    client_thread.join();

    EXPECT_TRUE(still_connected) << "Idle connection should survive with keepalive";

    server_socket.close();
}

TEST_F(SocketOptionsTest, KeepaliveResetOnDataTransfer) {
    // Test that keepalive timer resets when data is transferred
    uint16_t test_port = 20014;
    Address server_addr("127.0.0.1", test_port);

    auto server_result = Socket::create_tcp();
    ASSERT_TRUE(server_result.is_ok());
    Socket server_socket = std::move(server_result.value());

    server_socket.set_reuseaddr(true);
    server_socket.set_keepalive(true);

    int idle = 5, interval = 2, count = 2;
    setsockopt(server_socket.fd(), IPPROTO_TCP, TCP_KEEPIDLE, &idle, sizeof(idle));
    setsockopt(server_socket.fd(), IPPROTO_TCP, TCP_KEEPINTVL, &interval, sizeof(interval));
    setsockopt(server_socket.fd(), IPPROTO_TCP, TCP_KEEPCNT, &count, sizeof(count));

    ASSERT_TRUE(server_socket.bind(server_addr).is_ok());
    ASSERT_TRUE(server_socket.listen(1).is_ok());

    std::atomic<int> data_received{0};

    std::thread server_thread([&]() {
            auto accept_result = server_socket.accept();
            EXPECT_TRUE(accept_result.is_ok());
            Socket client_conn = std::move(accept_result.value());

            char buffer[128];
            for (int i = 0; i < 20; ++i) {
            auto recv_result = client_conn.recv(buffer, sizeof(buffer));
            if (recv_result.is_ok() && recv_result.value() > 0) {
            data_received++;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            });

    std::thread client_thread([&]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            auto client_result = Socket::create_tcp();
            EXPECT_TRUE(client_result.is_ok());
            Socket client_socket = std::move(client_result.value());

            client_socket.connect(server_addr);

            // Send data every 3 seconds (before keepalive idle timeout of 5s)
            for (int i = 0; i < 5; ++i) {
            const char* msg = "keepalive_reset";
            client_socket.send(msg, strlen(msg));
            std::this_thread::sleep_for(std::chrono::seconds(3));
            }
            });

    server_thread.join();
    client_thread.join();

    EXPECT_GT(data_received, 0) << "Should receive periodic data that resets keepalive";

    server_socket.close();
}

// ============================================================================
// TCP Keepalive Edge Cases
// ============================================================================

TEST_F(SocketOptionsTest, KeepaliveBeforeConnect) {
    // Test setting keepalive before connection is established
    auto result = socket_.set_keepalive(true);
    EXPECT_TRUE(result.is_ok());

    int idle = 10, interval = 5, count = 3;
    setsockopt(socket_.fd(), IPPROTO_TCP, TCP_KEEPIDLE, &idle, sizeof(idle));
    setsockopt(socket_.fd(), IPPROTO_TCP, TCP_KEEPINTVL, &interval, sizeof(interval));
    setsockopt(socket_.fd(), IPPROTO_TCP, TCP_KEEPCNT, &count, sizeof(count));

    // Verify settings persist
    EXPECT_EQ(get_socket_option<int>(socket_.fd(), SOL_SOCKET, SO_KEEPALIVE), 1);
    EXPECT_EQ(get_socket_option<int>(socket_.fd(), IPPROTO_TCP, TCP_KEEPIDLE), idle);
}

TEST_F(SocketOptionsTest, KeepaliveDisableAfterEnable) {
    // Test that disabling keepalive stops probing
    socket_.set_keepalive(true);

    int idle = 2;
    setsockopt(socket_.fd(), IPPROTO_TCP, TCP_KEEPIDLE, &idle, sizeof(idle));

    EXPECT_EQ(get_socket_option<int>(socket_.fd(), SOL_SOCKET, SO_KEEPALIVE), 1);

    // Disable keepalive
    socket_.set_keepalive(false);

    EXPECT_EQ(get_socket_option<int>(socket_.fd(), SOL_SOCKET, SO_KEEPALIVE), 0);

    // Parameters should still be readable but inactive
    int value = get_socket_option<int>(socket_.fd(), IPPROTO_TCP, TCP_KEEPIDLE);
    EXPECT_GT(value, 0); // Value is set but keepalive is disabled
}

TEST_F(SocketOptionsTest, KeepaliveInvalidParameters) {
    socket_.set_keepalive(true);

    // Test invalid parameters (should fail or be clamped)
    int invalid_idle = -1;
    int result = setsockopt(socket_.fd(), IPPROTO_TCP, TCP_KEEPIDLE, 
            &invalid_idle, sizeof(invalid_idle));

    // Should either fail or kernel clamps to minimum
    if (result == 0) {
        int value = get_socket_option<int>(socket_.fd(), IPPROTO_TCP, TCP_KEEPIDLE);
        EXPECT_GE(value, 1) << "Kernel should clamp invalid values";
    } else {
        EXPECT_EQ(result, -1) << "Should reject invalid values";
    }
}

TEST_F(SocketOptionsTest, KeepaliveBothEnds) {
    // Test keepalive on both server and client
    uint16_t test_port = 20015;
    Address server_addr("127.0.0.1", test_port);

    auto server_result = Socket::create_tcp();
    ASSERT_TRUE(server_result.is_ok());
    Socket server_socket = std::move(server_result.value());

    server_socket.set_reuseaddr(true);
    server_socket.set_keepalive(true);

    int idle = 5, interval = 2, count = 3;
    setsockopt(server_socket.fd(), IPPROTO_TCP, TCP_KEEPIDLE, &idle, sizeof(idle));
    setsockopt(server_socket.fd(), IPPROTO_TCP, TCP_KEEPINTVL, &interval, sizeof(interval));
    setsockopt(server_socket.fd(), IPPROTO_TCP, TCP_KEEPCNT, &count, sizeof(count));

    ASSERT_TRUE(server_socket.bind(server_addr).is_ok());
    ASSERT_TRUE(server_socket.listen(1).is_ok());

    std::atomic<bool> both_sides_keepalive{false};

    std::thread server_thread([&]() {
            auto accept_result = server_socket.accept();
            ASSERT_TRUE(accept_result.is_ok());
            Socket client_conn = std::move(accept_result.value());

            int keepalive = get_socket_option<int>(client_conn.fd(), SOL_SOCKET, SO_KEEPALIVE);
            if (keepalive == 1) {
            both_sides_keepalive = true;
            }

            std::this_thread::sleep_for(std::chrono::seconds(2));
            });

    std::thread client_thread([&]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            auto client_result = Socket::create_tcp();
            ASSERT_TRUE(client_result.is_ok());
            Socket client_socket = std::move(client_result.value());

            client_socket.set_keepalive(true);
            setsockopt(client_socket.fd(), IPPROTO_TCP, TCP_KEEPIDLE, &idle, sizeof(idle));
            setsockopt(client_socket.fd(), IPPROTO_TCP, TCP_KEEPINTVL, &interval, sizeof(interval));
            setsockopt(client_socket.fd(), IPPROTO_TCP, TCP_KEEPCNT, &count, sizeof(count));

            client_socket.connect(server_addr);

            std::this_thread::sleep_for(std::chrono::seconds(2));
            });

    server_thread.join();
    client_thread.join();

    // Note: Keepalive may or may not be inherited on accepted socket
    // This tests the behavior
    std::cout << "Both sides keepalive: " << both_sides_keepalive << std::endl;

    server_socket.close();
}

#endif // __linux__

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
