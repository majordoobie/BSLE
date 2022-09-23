#include <gtest/gtest.h>
#include <server_file_api.h>
#include <libgen.h>
#include <stdio.h>

extern "C"
{
    // Private structure declared again here to access members
    struct verified_path
    {
        char * p_path;
    };
}

static bool run_tests = true;

TEST(TestForTestFiles, Test)
{
    // Creates the test files for this unit test
    //mkdir -p /tmp/dir/another_dir; touch /tmp/dir/somefile.txt; touch /tmp/otherfile.txt";
    if (-1 == mkdir("/tmp/dir", 0777))
    {
        if (EEXIST != errno)
        {
            run_tests = false;
            ASSERT_TRUE(run_tests) << "Could not create test files\n";
        }
    }
    if (-1 == mkdir("/tmp/dir/another_dir", 0777))
    {
        if (EEXIST != errno)
        {
            perror("mkdir");
            run_tests = false;
            ASSERT_TRUE(run_tests) << "Could not create test files\n";
        }
    }
    FILE * f = fopen("/tmp/dir/somefile.txt", "w");
    if (NULL == f)
    {
        perror("open");
        run_tests = false;
        ASSERT_TRUE(run_tests) << "Could not create test files\n";
    }
    fclose(f);
    f = fopen("/tmp/dir/another_dir/another_file.txt", "w");
    if (NULL == f)
    {
        perror("open");
        run_tests = false;
        ASSERT_TRUE(run_tests) << "Could not create test files\n";
    }
    fclose(f);
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
    if (!run_tests)
    {
        GTEST_SKIP_("Did not find test files\n");
    }
    auto [parent, child, expected_str, b_expect_resolve, expect_find, expect_may_exist] = GetParam();
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

TEST_P(ServerFileApiTest, TestFileMayExist)
{
    if (!run_tests)
    {
        GTEST_SKIP_("Did not find test files\n");
    }

    auto [parent, child, expected_str, b_expect_resolve, expect_find, expect_may_exist] = GetParam();

    static int val = 0;
    val++;

    verified_path_t * p_path = f_dir_resolve((char *)parent.c_str(), (char *)child.c_str());
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
        std::make_tuple("/tmp/dir", "../dir/somefile.txt", "/tmp/dir/somefile.txt", true, true, true)
    ));
