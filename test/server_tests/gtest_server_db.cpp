#include <gtest/gtest.h>
#include <server_db.h>
#include <filesystem>
#include <fstream>

const char * home = "/tmp";

TEST(TestDBParsing, ParseDB)
{
    // Remove the cape directory if it exists
    std::filesystem::remove_all("/tmp/.cape");

    /*
     * Test the successful creation of the ./cape dir and the .cape/.cape.db and
     * .cape/.cape.hash
     */
    int res = hash_init_db((char *)home, strlen(home));
    EXPECT_EQ(res, 0); // Creates both

//    std::filesystem::remove_all("/tmp/.cape");
}
/*
 * All these tests have to run in order since they access files on disk.
 * Therefore instead of making a test fixture (I tried, but it breaks when running
 * tests with -j) the tests are all within one test unit
 */
TEST(TestDBInit, SingleThreadTests)
{
    // Remove the cape directory if it exists
    std::filesystem::remove_all("/tmp/.cape");
    int res = 0;

    /*
     * Test the successful creation of the ./cape dir and the .cape/.cape.db and
     * .cape/.cape.hash
     */
    res = hash_init_db((char *)home, strlen(home));
    EXPECT_EQ(res, 0); // Creates both
    std::filesystem::remove_all("/tmp/.cape");

    //Cleanup

    /*
     * Test that when one of the two mandatory files are missing that the
     * unit fails
     */
    res = hash_init_db((char *)home, strlen(home));
    EXPECT_EQ(res, 0); // Fails because one of the files is missing
    std::filesystem::remove("/tmp/.cape/.cape.db");
    res = hash_init_db((char *)home, strlen(home));
    EXPECT_EQ(res, -1); // Fails because one of the files is missing
    std::filesystem::remove_all("/tmp/.cape");


    /*
     * Expect failure when the hash file does NOT have the magic bytes
     * making it an invalid file
     */
    res = hash_init_db((char *)home, strlen(home));
    EXPECT_EQ(res, 0); // Fails because one of the files is missing
    std::filesystem::remove("/tmp/.cape/.cape.hash");
    std::ofstream output("/tmp/.cape/.cape.hash");
    res = hash_init_db((char *)home, strlen(home));
    EXPECT_EQ(res, -1); // Identifies that the db file DOES NOT have the MAGIC
    std::filesystem::remove_all("/tmp/.cape");


    /*
     * Expect failure due to the hash of the .cape.db not matching the hash
     * in the .cape.hash
     */
    res = hash_init_db((char *)home, strlen(home));
    EXPECT_EQ(res, 0); // Fails because one of the files is missing
    std::filesystem::remove("/tmp/.cape/.cape.db");
    output << 0xFFAAFABA;
    res = hash_init_db((char *)home, strlen(home));
    EXPECT_EQ(res, -1);
    std::filesystem::remove_all("/tmp/.cape");
}
