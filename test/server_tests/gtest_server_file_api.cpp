#include <gtest/gtest.h>
#include <server_file_api.h>

extern "C"
{
FILE * get_file(const char * p_home_dir,
                size_t home_length,
                const char * p_file,
                const char * p_read_mode);
char * join_paths(const char * p_root,
                  size_t root_length,
                  const char * p_child,
                  size_t child_length);
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
    auto [parent, child, expected, expect_resolve, expect_find] = GetParam();

    char * joined_path = join_paths(
        (char *)parent.c_str(),
        parent.size(),
        (char *)child.c_str(),
        child.size()
    );


    if (expect_resolve)
    {
        ASSERT_NE(nullptr, joined_path) << "\n[!!] Create test files; mkdir /tmp/dir; touch /tmp/dir/somefile.txt; touch /tmp/otherfile.txt";
        EXPECT_EQ(0, strcmp(expected.c_str(), joined_path));

        FILE * file = get_file(parent.c_str(),
                               parent.size(),
                               (const char *)joined_path,
                               "r");

        if (expect_find)
        {
            EXPECT_NE(nullptr, file);
        }
        else
        {
            EXPECT_EQ(nullptr, file);
        }
        if (NULL != file)
        {
            fclose(file);
        }
    }
    else
    {
        EXPECT_EQ(nullptr, joined_path);
    }


    free(joined_path);
}

INSTANTIATE_TEST_SUITE_P(
    TestJoin,
    ServerFileApiTest,
    ::testing::Values(
        std::make_tuple("/tmp/dir", "somefile.txt", "/tmp/dir/somefile.txt", true, true),
        std::make_tuple("/tmp/dir", "somefile.txt", "/tmp/dir/somefile.txt", true, true),
        std::make_tuple("/tmp/dir/", "/somefile.txt", "/tmp/dir/somefile.txt", true, true),
        std::make_tuple("/tmp/dir/../dir/", "../dir/somefile.txt", "/tmp/dir/somefile.txt", true, false),
        std::make_tuple("/tmp/dir/", "../otherfile.txt", "/tmp/otherfile.txt", true, false),
        std::make_tuple("/tmp/dir/", "no_exist.txt", "", false, false),
        std::make_tuple("/tmp/dir", "../dir/somefile.txt", "/tmp/dir/somefile.txt", true, true)
    ));

