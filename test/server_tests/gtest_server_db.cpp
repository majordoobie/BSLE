#include <gtest/gtest.h>
#include <server_db.h>
#include <filesystem>
#include <fstream>

const char * home = "/tmp";

/*!
 * Test ability to properly parse the db file on disk with the defaults
 */
TEST(TestDBParsing, ParseDB)
{
    // Remove the cape directory if it exists
    std::filesystem::remove_all("/tmp/.cape");
    verified_path_t * p_home_dir = f_set_home_dir(home, strlen(home));

    /*
     * Test the successful creation of the ./cape dir and the .cape/.cape.db and
     * .cape/.cape.hash
     */
    htable_t * htable = db_init(p_home_dir);
    EXPECT_NE(htable, nullptr); // Creates both
    db_shutdown(htable, p_home_dir);
    f_destroy_path(&p_home_dir);
    std::filesystem::remove_all("/tmp/.cape");
}

/*!
 * All these tests have to run in order since they access files on disk.
 * Therefore instead of making a test fixture (I tried, but it breaks when running
 * tests with -j) the tests are all within one test unit
 */
TEST(TestDBInit, SingleThreadTests)
{
    // Remove the cape directory if it exists
    std::filesystem::remove_all("/tmp/.cape");
    verified_path_t * p_home_dir = f_set_home_dir(home, strlen(home));
    htable_t * htable = NULL;

    /*
     * Test the successful creation of the ./cape dir and the .cape/.cape.db and
     * .cape/.cape.hash
     */
    htable = db_init(p_home_dir);
    EXPECT_NE(htable, nullptr); // Creates both
    std::filesystem::remove_all("/tmp/.cape");

    //Cleanup

    /*
     * Test that when one of the two mandatory files are missing that the
     * unit fails
     */
    htable = db_init(p_home_dir);
    EXPECT_NE(htable, nullptr); // Fails because one of the files is missing
    std::filesystem::remove("/tmp/.cape/.cape.db");
    htable = db_init(p_home_dir);
    EXPECT_EQ(htable, nullptr); // Fails because one of the files is missing
    std::filesystem::remove_all("/tmp/.cape");


    /*
     * Expect failure when the hash file does NOT have the magic bytes
     * making it an invalid file
     */
    htable = db_init(p_home_dir);
    EXPECT_EQ(htable, nullptr); // Fails because one of the files is missing
    std::filesystem::remove("/tmp/.cape/.cape.hash");
    std::ofstream output("/tmp/.cape/.cape.hash");
    htable = db_init(p_home_dir);
    EXPECT_EQ(htable, nullptr); // Identifies that the db file DOES NOT have the MAGIC
    std::filesystem::remove_all("/tmp/.cape");


    /*
     * Expect failure due to the hash of the .cape.db not matching the hash
     * in the .cape.hash
     */
    htable = db_init(p_home_dir);
    EXPECT_EQ(htable, nullptr); // Fails because one of the files is missing
    std::filesystem::remove("/tmp/.cape/.cape.db");
    output << 0xFFAAFABA;
    htable = db_init(p_home_dir);
    EXPECT_EQ(htable, nullptr);
    std::filesystem::remove_all("/tmp/.cape");
}

/*!
 * Test ability to properly parse the db file on disk with the defaults
 */
TEST(TestDBParsing, UserAdd)
{
    // Remove the cape directory if it exists
    std::filesystem::remove_all("/tmp/.cape");
    verified_path_t * p_home_dir = f_set_home_dir(home, strlen(home));

    /*
     * Test the successful creation of the ./cape dir and the .cape/.cape.db and
     * .cape/.cape.hash
     */
    htable_t * htable = db_init(p_home_dir);
    EXPECT_NE(htable, nullptr); // Creates both

    server_error_codes_t res = db_create_user(htable, "VooDooRanger", "New Belgium", READ);
    EXPECT_EQ(res, OP_SUCCESS);
    res = db_create_user(htable, "VoodooRanger", "New Belgium", READ);
    EXPECT_EQ(res, OP_SUCCESS);
    res = db_create_user(htable, "Voodoo", "New Belgium Brew", READ);
    EXPECT_EQ(res, OP_SUCCESS);

    db_shutdown(htable, p_home_dir);
    f_destroy_path(&p_home_dir);
//    std::filesystem::remove_all("/tmp/.cape");
}
