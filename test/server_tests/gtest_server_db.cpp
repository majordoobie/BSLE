#include <gtest/gtest.h>
#include <server_db.h>
#include <filesystem>
#include <fstream>


/*!
 * Test ability to properly parse the db file on disk with the defaults
 */
TEST(TestDBParsing, ParseDB)
{
    // Remove the cape directory if it exists
    const char * home = "/tmp/test_parse_db";
    std::filesystem::remove_all(home);
    std::filesystem::create_directory(home);
    verified_path_t * p_home_dir = f_set_home_dir(home, strlen(home));

    /*
     * Test the successful creation of the ./cape dir and the .cape/.cape.db and
     * .cape/.cape.hash
     */
    htable_t * htable = db_init(p_home_dir);
    EXPECT_NE(htable, nullptr); // Creates both
    db_shutdown(p_home_dir, htable);
    f_destroy_path(&p_home_dir);
    std::filesystem::remove_all(home);
}

/*!
 * All these tests have to run in order since they access files on disk.
 * Therefore instead of making a test fixture (I tried, but it breaks when running
 * tests with -j) the tests are all within one test unit
 */
TEST(TestDBInit, SingleThreadTests)
{
    // Remove the cape directory if it exists
    const char * home = "/tmp/test_single_thread_tests";
    const char * home_cape = "/tmp/test_single_thread_tests/.cape";
    const char * home_cape_db = "/tmp/test_single_thread_tests/.cape/.cape.db";
    const char * home_cape_hash = "/tmp/test_single_thread_tests/.cape/.cape.hash";

    std::filesystem::remove_all(home);
    std::filesystem::create_directory(home);

    // Remove the cape directory if it exists
    verified_path_t * p_home_dir = f_set_home_dir(home, strlen(home));
    htable_t * htable = NULL;

    /*
     * Test the successful creation of the ./cape dir and the .cape/.cape.db and
     * .cape/.cape.hash
     */
    htable = db_init(p_home_dir);
    EXPECT_NE(htable, nullptr); // Creates both
    std::filesystem::remove_all(home_cape);
    htable_destroy(htable, HT_FREE_PTR_FALSE, HT_FREE_PTR_TRUE);

    /*
     * Test that when one of the two mandatory files are missing that the
     * unit fails
     */
    htable = db_init(p_home_dir);
    EXPECT_NE(htable, nullptr);
    // Remove the db file to force a failure
    std::filesystem::remove(home_cape_db);
    htable_destroy(htable, HT_FREE_PTR_FALSE, HT_FREE_PTR_TRUE);
    htable = db_init(p_home_dir);
    EXPECT_EQ(htable, nullptr); // Fails because one of the files is missing
    std::filesystem::remove_all(home_cape);


    /*
     * Expect failure when the hash file does NOT have the magic bytes
     * making it an invalid file
     */
    htable = db_init(p_home_dir);
    EXPECT_NE(htable, nullptr);
    htable_destroy(htable, HT_FREE_PTR_FALSE, HT_FREE_PTR_TRUE);
    // Remove the contents of the hash file to force a failure of hash match
    std::filesystem::remove(home_cape_hash);
    std::ofstream output(home_cape_hash);
    htable = db_init(p_home_dir);
    EXPECT_EQ(htable, nullptr); // Identifies that the db file DOES NOT have the MAGIC
    std::filesystem::remove_all(home_cape);


    /*
     * Expect failure due to the hash of the .cape.db not matching the hash
     * in the .cape.hash
     */
    htable = db_init(p_home_dir);
    EXPECT_NE(htable, nullptr);
    htable_destroy(htable, HT_FREE_PTR_FALSE, HT_FREE_PTR_TRUE);

    std::filesystem::remove(home_cape_db);
    output << 0xFFAAFABA;
    htable = db_init(p_home_dir);
    EXPECT_EQ(htable, nullptr);

    std::filesystem::remove_all(home);
    f_destroy_path(&p_home_dir);
}

/*!
 * Test ability to create a new user and update the database with the new
 * user added
 */
TEST(TestDBParsing, UserAdd)
{
    // Remove the cape directory if it exists
    const char * home = "/tmp/test_user_add";
    std::filesystem::remove_all(home);
    std::filesystem::create_directory(home);

    // Remove the cape directory if it exists
    verified_path_t * p_home_dir = f_set_home_dir(home, strlen(home));

    /*
     * Test the successful creation of the ./cape dir and the .cape/.cape.db and
     * .cape/.cape.hash
     */
    htable_t * htable = db_init(p_home_dir);
    EXPECT_NE(htable, nullptr); // Creates both

    server_error_codes_t res = db_create_user( p_home_dir, htable,
                                               "VooDooRanger", "New Belgium", READ);
    EXPECT_EQ(res, OP_SUCCESS);
    res = db_create_user(p_home_dir, htable, "VoodooRanger", "New Belgium CO", READ);
    EXPECT_EQ(res, OP_SUCCESS);
    res = db_create_user(p_home_dir, htable, "Voodoo", "New Belgium Brew", READ);
    EXPECT_EQ(res, OP_SUCCESS);
    res = db_create_user(p_home_dir, htable, "Voodoo", "New Belgium Brew", READ);
    EXPECT_EQ(res, OP_USER_EXISTS);

    db_shutdown(p_home_dir, htable);

    // re init to make sure that all the users persisted
    htable = db_init(p_home_dir);
    EXPECT_EQ(htable_get_length(htable), 4);
    db_shutdown(p_home_dir, htable);

    f_destroy_path(&p_home_dir);
    std::filesystem::remove_all(home);
}
