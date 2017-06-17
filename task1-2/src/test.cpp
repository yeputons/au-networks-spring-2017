#include <iostream>
#include <cstdint>
#include <cstddef>
#include <memory>
#include <cstring>
#include <pthread.h>
#include <gtest/gtest.h>

#define TEST_TCP_STREAM_SOCKET
#define TEST_AU_STREAM_SOCKET

#include "stream_socket.h"
#ifdef TEST_TCP_STREAM_SOCKET
#include "tcp_socket.h"
#endif
#ifdef TEST_AU_STREAM_SOCKET
#include "au_stream_socket.h"
#endif

const char *TEST_ADDR = "localhost";
#ifdef TEST_TCP_STREAM_SOCKET
const tcp_port TCP_TEST_PORT = 40002;
#endif
#ifdef TEST_AU_STREAM_SOCKET
const au_stream_port AU_TEST_CLIENT_PORT = 40001;
const au_stream_port AU_TEST_SERVER_PORT = 301;
#endif

class StreamSocketsTest : public testing::Test {
public:
virtual void SetUp() = 0;

protected:
std::unique_ptr<stream_client_socket> client;
std::unique_ptr<stream_server_socket> server;
std::unique_ptr<stream_socket> server_client;

#define STREAM_TEST_VOLUME (1024 * 1024 * 1 / 4)

static void* test_stream_sockets_datapipe_thread_func(void* obj)
{
    uint64_t i = 0;
    const uint64_t max_i = STREAM_TEST_VOLUME / sizeof(uint64_t);
    constexpr size_t BUF_ITEMS = 1024;
    uint64_t buf[BUF_ITEMS];

    bool *result = new bool(true);

    auto &client = static_cast<StreamSocketsTest*>(obj)->client;
    client->connect();
    while (i < max_i) {
        client->recv(buf, sizeof(buf));
        for (size_t buf_ix = 0; buf_ix < BUF_ITEMS; ++buf_ix, ++i) {
            if (buf[buf_ix] != i) {
                std::cout << i << " " << buf[buf_ix] << std::endl;
                *result = false;
                return result;
            }
        }
    }

    return result;
}

void test_stream_sockets_datapipe()
{
    uint64_t i = 0;
    const uint64_t max_i = STREAM_TEST_VOLUME / sizeof(uint64_t);
    const size_t i_portion = 1024;
    uint64_t buf[i_portion];

    pthread_t th;
    pthread_create(&th, NULL, test_stream_sockets_datapipe_thread_func, static_cast<StreamSocketsTest*>(this));

    server_client.reset(server->accept_one_client());
    while (i < max_i) {
        for (size_t buf_ix = 0; buf_ix < i_portion; ++buf_ix, ++i)
            buf[buf_ix] = i;
        server_client->send(buf, sizeof(buf));
    }
    void *result_ptr;
    pthread_join(th, &result_ptr);
    bool *result = static_cast<bool*>(result_ptr);
    ASSERT_TRUE(*result);
    delete result;
}

static void* test_stream_sockets_partial_data_sent_thread_func(void *obj)
{
    char buf[4];

    auto &client = static_cast<StreamSocketsTest*>(obj)->client;
    client->connect();
    buf[0] = 'H';
    buf[1] = 'e';
    buf[2] = 'l';
    buf[3] = 'l';
    client->send(buf, 2);
    client.reset();

    return NULL;
}

void test_stream_sockets_partial_data_sent()
{
    char buf[4];

    pthread_t th;
    pthread_create(&th, NULL, test_stream_sockets_partial_data_sent_thread_func, static_cast<StreamSocketsTest*>(this));

    server_client.reset(server->accept_one_client());
    bool thrown = false;
    try {
        server_client->recv(buf, 4);
    } catch (...) {
        // No data in the socket now
        // Check that error is returned
        thrown = true;
    }
    EXPECT_TRUE(thrown);
    pthread_join(th, NULL);
}

};  // class StreamSocketsTest

#ifdef TEST_TCP_STREAM_SOCKET
class TcpStreamSocketsTest : public StreamSocketsTest {
    void SetUp() override {
        server.reset(new tcp_server_socket(TEST_ADDR, TCP_TEST_PORT));
        client.reset(new tcp_client_socket(TEST_ADDR, TCP_TEST_PORT));
    }
};

TEST_F(TcpStreamSocketsTest, Datapipe) {
    test_stream_sockets_datapipe();
}

TEST_F(TcpStreamSocketsTest, PartialDataSent) {
    test_stream_sockets_partial_data_sent();
}
#endif

#ifdef TEST_AU_STREAM_SOCKET
class AuStreamSocketsTest : public StreamSocketsTest {
    void SetUp() override {
        server.reset(new au_stream_server_socket(TEST_ADDR, AU_TEST_SERVER_PORT));
        client.reset(new au_stream_client_socket(TEST_ADDR, AU_TEST_CLIENT_PORT, AU_TEST_SERVER_PORT));
    }
};

TEST_F(AuStreamSocketsTest, Datapipe) {
    test_stream_sockets_datapipe();
}

TEST_F(AuStreamSocketsTest, PartialDataSent) {
    test_stream_sockets_partial_data_sent();
}
#endif
