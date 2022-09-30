#include <gtest/gtest.h>
#include <server_sock.h>


TEST(TestServerCTRL, TestDIR)
{
    start_server(0, 8888, nullptr);
}