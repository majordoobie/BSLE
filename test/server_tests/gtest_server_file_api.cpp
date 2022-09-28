#include <gtest/gtest.h>
#include <server_file_api.h>
#include <stdio.h>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <atomic>

extern "C"
{
    // Private structure declared again here to access members
    struct verified_path
    {
        char * p_path;
    };
}

// Variables are used to synchronize the threads in ctest -j $(nproc)
static volatile std::atomic_uint clear = 0;
static unsigned int tests = 2;

/*!
 * Create the test files if they do not exist
 */
void init_test_dir(void)
{
    const std::filesystem::path test_dir{"/tmp/dir"};

    if (!std::filesystem::exists(test_dir))
    {
        std::filesystem::create_directory(test_dir);
    }
    if (!std::filesystem::exists(test_dir/"another_dir"))
    {
        std::filesystem::create_directory(test_dir/"another_dir");
    }
    if (!std::filesystem::exists(test_dir/"somefile.txt"))
    {
        std::ofstream {test_dir/"somefile.txt"};
    }
    if (!std::filesystem::exists(test_dir/"another_dir/another_file.txt"))
    {
        std::ofstream{test_dir/"another_dir/another_file.txt"};
    }
}
class ServerFileApiTest : public ::testing::TestWithParam<std::tuple<std::string, std::string, std::string, bool, bool, bool>>{};

/*
 * I took the output of:
 * echo -n "password" | sha256sum
 * And pasted the output into "sha1sum" then converted that string into a
 * byte array and compared it to the hashing of "password" to ensure that
 * both outputs are the same
 */
TEST_P(ServerFileApiTest, TestFileJoining)
{
    init_test_dir();

    auto [parent, child, expected_str, b_expect_resolve, expect_find, expect_may_exist] = GetParam();

    // Cleaup if the tests are over
    if (parent == "NOT_TEST")
    {
        // Atomic function to clear the test directory. This is used to
        // synchronize the threads for ctest -j $(nrpoc)
        // note that we are evaluating tests - 1 because fetch_add returns the
        // previous value before it was incremented
        long long int val = clear.fetch_add(1);
        printf("The value of val is: %lld\n", val);
        if (val >= (tests - 1))
        {
            printf("Cleaning up test dir\n");
            std::filesystem::remove_all("/tmp/dir");
        }
    }
    else
    {
        verified_path_t * p_path = f_path_resolve(parent.c_str(), child.c_str());

        if (b_expect_resolve)
        {
            ASSERT_NE(nullptr, p_path) << "\n[!!] Create test files; mkdir -p /tmp/dir/another_dir; touch /tmp/dir/somefile.txt; touch /tmp/otherfile.txt";
            EXPECT_EQ(0, strcmp(expected_str.c_str(), p_path->p_path));


            FILE * h_file = f_open_file(p_path, "r");

            if (expect_find)
            {
                EXPECT_NE(nullptr, h_file);
            }
            else
            {
                EXPECT_EQ(nullptr, h_file);
            }
            if (NULL != h_file)
            {
                fclose(h_file);
            }
            f_destroy_path(&p_path);
        }
        else
        {
            EXPECT_EQ(nullptr, p_path);
        }
        free(p_path);
    }
}

TEST_P(ServerFileApiTest, TestFileMayExist)
{
    init_test_dir();

    auto [parent, child, expected_str, b_expect_resolve, expect_find, expect_may_exist] = GetParam();
    static int v = 0;
    v++;

    // Cleaup if the tests are over
    if (parent == "NOT_TEST")
    {
        // Atomic function to clear the test directory. This is used to
        // synchronize the threads for ctest -j $(nrpoc)
        // note that we are evaluating tests - 1 because fetch_add returns the
        // previous value before it was incremented
        long long int val = clear.fetch_add(1);
        printf("The value of val is: %lld\n", val);
        if (val >= (tests - 1))
        {
            printf("Cleaning up test dir\n");
            std::filesystem::remove_all("/tmp/dir");
        }
    }

    else
    {
        verified_path_t * p_path =
            f_valid_resolve((char *)parent.c_str(), (char *)child.c_str());
        if (expect_may_exist)
        {
            EXPECT_NE(nullptr, p_path);
            f_destroy_path(&p_path);
        }
        else
        {
            EXPECT_EQ(nullptr, p_path);
        }
    }
}

INSTANTIATE_TEST_SUITE_P(
    TestJoin,
    ServerFileApiTest,
    ::testing::Values(
        std::make_tuple("/tmp/dir", "somefile.txt", "/tmp/dir/somefile.txt", true, true, true),
        std::make_tuple("/tmp/dir/", "/somefile.txt", "/tmp/dir/somefile.txt", true, true, true),
        std::make_tuple("/tmp/dir", "/somefile.txt", "/tmp/dir/somefile.txt", true, true, true),
        std::make_tuple("/tmp/dir/", "somefile.txt", "/tmp/dir/somefile.txt", true, true, true),
        std::make_tuple("/tmp/dir/", "../../tmp/dir/somefile.txt", "/tmp/dir/somefile.txt", true, true, true),
        std::make_tuple("/tmp/dir/", "../otherfile.txt", "/tmp/otherfile.txt", false, false, false),
        std::make_tuple("/tmp/dir/", "", "", false, false, false),
        std::make_tuple("/tmp/dir/", "     ", "", false, false, true),
        std::make_tuple("/tmp/dir/", "test file", "", false, false, true),
        std::make_tuple("/tmp/dir/", "no_exist.txt", "/tmp/dir/no_exists.txt", false, false, true),
        std::make_tuple("/tmp/dir/", "/another_dir/no_exist.txt", "/tmp/dir/no_exists.txt", false, false, true),
        std::make_tuple("/tmp/dir", "../dir/somefile.txt", "/tmp/dir/somefile.txt", true, true, true),
        std::make_tuple("NOT_TEST", "NOT_TEST", "NOT_TEST", true, true, true)
    ));

// These tests have to be performed in order because of the ordering
TEST(TestFileApi, InSequence)
{
    // Create the test directory
    const std::filesystem::path test_dir{"/tmp/in_sequence"};
    std::filesystem::remove_all(test_dir);
    std::filesystem::create_directory(test_dir);

    // Create the directory
    verified_path_t * p_db_dir = f_valid_resolve(test_dir.c_str(), "dir_one");
    server_error_codes_t status = f_create_dir(p_db_dir);
    EXPECT_EQ(status, OP_SUCCESS);

    // Add a file to the directory and try to delete the directory
    std::ofstream {test_dir/"dir_one/somefile.txt"};
    status = f_del_file(p_db_dir);
    EXPECT_EQ(status, OP_DIR_NOT_EMPTY);

    // Delete the file first, then try to delete the directory
    verified_path_t * p_test_file = f_valid_resolve(test_dir.c_str(), "dir_one/somefile.txt");
    status = f_del_file(p_test_file);
    EXPECT_EQ(status, OP_SUCCESS);
    status = f_del_file(p_db_dir);
    EXPECT_EQ(status, OP_SUCCESS);

    f_destroy_path(&p_db_dir);
    f_destroy_path(&p_test_file);
    std::filesystem::remove_all(test_dir);
}