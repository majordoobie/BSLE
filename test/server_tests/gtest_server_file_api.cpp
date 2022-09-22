#include <gtest/gtest.h>
#include <server_file_api.h>
#include <libgen.h>

extern "C"
{
    // Private structure declared again here to access members
    struct verified_path
    {
        char * p_path;
    };
}

class ServerFileApiTest : public ::testing::TestWithParam<std::tuple<std::string, std::string, std::string, bool, bool>>{};

/*
 * I took the output of:
 * echo -n "password" | sha256sum
 * And pasted the output into "sha1sum" then converted that string into a
 * byte array and compared it to the hashing of "password" to ensure that
 * both outputs are the same
 */
TEST_P(ServerFileApiTest, TestFileJoining)
{
    auto [parent, child, b_expected, b_expect_resolve, expect_find] = GetParam();

//    static int val = 0;
//    if (0 == val)
//        printf("Orig\t\t\t\t\t\t\t\tDirName\t\t\t\t\t\t\t\tDirBase\n");
//    val++;
//
//    char * first = strdup((char *)child.c_str());
//    char * second = strdup((char *)child.c_str());
//
//    printf("%s\t\t\t\t\t\t\t\t%s\t\t\t\t\t\t\t\t%s\n", child.c_str(), dirname(first), basename(second));
//    free(first);
//    free(second);
//    return;

    verified_path_t * p_path = f_path_resolve(parent.c_str(), child.c_str());

    if (b_expect_resolve)
    {
        ASSERT_NE(nullptr, p_path) << "\n[!!] Create test files; mkdir /tmp/dir; touch /tmp/dir/somefile.txt; touch /tmp/otherfile.txt";
        EXPECT_EQ(0, strcmp(b_expected.c_str(), p_path->p_path));


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

INSTANTIATE_TEST_SUITE_P(
    TestJoin,
    ServerFileApiTest,
    ::testing::Values(
        std::make_tuple("/tmp/dir", "somefile.txt", "/tmp/dir/somefile.txt", true, true),
        std::make_tuple("/tmp/dir/", "/somefile.txt", "/tmp/dir/somefile.txt", true, true),
        std::make_tuple("/tmp/dir", "/somefile.txt", "/tmp/dir/somefile.txt", true, true),
        std::make_tuple("/tmp/dir/", "somefile.txt", "/tmp/dir/somefile.txt", true, true),
        std::make_tuple("/tmp/dir/", "../../tmp/dir/somefile.txt", "/tmp/dir/somefile.txt", true, true),
        std::make_tuple("/tmp/dir/", "../otherfile.txt", "/tmp/otherfile.txt", false, false),
        std::make_tuple("/tmp/dir/", "no_exist.txt", "", false, false),
        std::make_tuple("/tmp/dir", "../dir/somefile.txt", "/tmp/dir/somefile.txt", true, true)
    ));

