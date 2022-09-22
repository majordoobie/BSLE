#include <gtest/gtest.h>
#include <arpa/inet.h>

#include <server_args.h>
#include <server_ctrl.h>

extern "C"
{
    uint32_t get_port(char * port);
    uint32_t get_timeout(char * timeout);
}

class ServerTestValidPorts : public ::testing::TestWithParam<std::tuple<std::string, bool>>{};

TEST_P(ServerTestValidPorts, TestValidPorts)
{
    auto [port_str, expect_failure] = GetParam();

    uint32_t port = get_port((char *)port_str.c_str());
    if (expect_failure)
    {
        EXPECT_EQ(port, 0);
    }
    else
    {
        EXPECT_TRUE(port >= MIN_PORT && port <= MAX_PORT);
    }
}

INSTANTIATE_TEST_SUITE_P(
    PortTest,
    ServerTestValidPorts,
    ::testing::Values(
        std::make_tuple("1", true),
        std::make_tuple("60000", false),
        std::make_tuple("65535", false),
        std::make_tuple("65536", true),
        std::make_tuple("0", true)
        ));

class ServerTestValidTimeouts : public ::testing::TestWithParam<std::tuple<std::string, bool>>{};

TEST_P(ServerTestValidTimeouts, TestValidTimeouts)
{
    auto [port_str, expect_failure] = GetParam();

    uint32_t timeout = get_timeout((char *)port_str.c_str());
    if (expect_failure)
    {
        EXPECT_EQ(timeout, 0);
    }
    else
    {
        EXPECT_TRUE(timeout >= 0 && timeout <= UINT32_MAX);
    }
}

INSTANTIATE_TEST_SUITE_P(
    TimeoutTest,
    ServerTestValidTimeouts,
    ::testing::Values(
        std::make_tuple("1", false),
        std::make_tuple("-1", true),
        std::make_tuple("4294967296", true),
        std::make_tuple("65536", false),
        std::make_tuple("0", true)
    ));

class ServerCmdTester : public ::testing::TestWithParam<std::tuple<std::vector<std::string>, bool>>{};

// Parameter test handles signed testing
TEST_P(ServerCmdTester, TestingServerArguments)
{
    // Reset getopt value for testing
    optind = 0;

    static int val = 0;
    val++;

    auto [str_vector, expect_failure] = GetParam();
    int arg_count = (int)str_vector.size();

    // Turn the string vector into an array of char array
    char ** argv = new char * [str_vector.size()];
    for(size_t i = 0; i < str_vector.size(); i++)
    {
        argv[i] = new char[str_vector[i].size() + 1];
        strcpy(argv[i], str_vector[i].c_str());
    }

    // Call the test function
    args_t * args = parse_args(arg_count, argv);
    if (expect_failure)
    {
        EXPECT_EQ(args, nullptr);
    }
    else
    {
        EXPECT_NE(args, nullptr);
    }


    // Free the array of char arrays
    for(size_t i = 0; i < str_vector.size(); i++)
    {
        delete [] argv[i];
    }
    delete [] argv;
    free_args(&args);
}

INSTANTIATE_TEST_SUITE_P(
    AdditionTests,
    ServerCmdTester,
    ::testing::Values(
        std::make_tuple(std::vector<std::string>{__FILE__, "-p", "31337"}, true),
        std::make_tuple(std::vector<std::string>{__FILE__, "-d", "/tmp", "-h"}, true),
        std::make_tuple(std::vector<std::string>{__FILE__, "-d", "/tmp", "-p", "1"}, true),
        std::make_tuple(std::vector<std::string>{__FILE__, "-d", "/tmp", "-p", "65535"}, false),
        std::make_tuple(std::vector<std::string>{__FILE__, "-d", "/tmp", "-p", "65536"}, true),
        std::make_tuple(std::vector<std::string>{__FILE__, "-d", "/tmp", "-p", "0"}, true),
        std::make_tuple(std::vector<std::string>{__FILE__, "-d", "/tmp", "-p", "4000", "-t", "8"}, false),
        std::make_tuple(std::vector<std::string>{__FILE__, "-d", "/tmp", "-p", "4000", "-t", "0"}, true),
        std::make_tuple(std::vector<std::string>{__FILE__, "-d", "/tmp", "-p", "4000", "-t", "-1"}, true),
        std::make_tuple(std::vector<std::string>{__FILE__, "-d", "/tmp", "-t", "8"}, false),
        std::make_tuple(std::vector<std::string>{__FILE__, "-d", "/tmp", "-t", "-8"}, true),
        std::make_tuple(std::vector<std::string>{__FILE__, "-d", "/tmp", "-t", "0"}, true),
        std::make_tuple(std::vector<std::string>{__FILE__, "-d", "/tmp", "-t", "1", "extra_arg"}, true),
        std::make_tuple(std::vector<std::string>{__FILE__, "-d", "/tmp", "-p", "4000", "-t", "8", "extra_arg"}, true),
        std::make_tuple(std::vector<std::string>{__FILE__, "-d", "/tmp", "-p", "4000", "-t", "8", "-t", "9000"}, true),
        std::make_tuple(std::vector<std::string>{__FILE__, "-d", "/tmp", "-p"}, true),
        std::make_tuple(std::vector<std::string>{__FILE__, "-d", "/tmp", "extra_arg"}, true),
        std::make_tuple(std::vector<std::string>{__FILE__, "-d", "/tmp", "-w"}, true),
        std::make_tuple(std::vector<std::string>{__FILE__, "-d", "/tmp", "-w", "10", "-p", "10"}, true),
        std::make_tuple(std::vector<std::string>{__FILE__}, true)
    ));

