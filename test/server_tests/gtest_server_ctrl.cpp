#include <gtest/gtest.h>
#include <server_ctrl.h>
#include <filesystem>
#include <fstream>

const char * home = "/tmp";

TEST(TestDBInit, TestInitAllMissing)
{
    // Remove the cape directory if it exists
    std::filesystem::remove_all("/tmp/.cape");
    int res = hash_init_db((char *)home, strlen(home));
    EXPECT_EQ(res, 0); // Creates both
}


class DBInitLogic : public ::testing::Test
{
 protected:
    void SetUp() override
    {
        std::filesystem::remove_all("/tmp/.cape");
        hash_init_db((char *)home, strlen(home));
    }
    void TearDown() override
    {
        std::filesystem::remove_all("/tmp/.cape");
    }
};

/*
 * This test is looking for failure to init because there is one of two
 * files missing
 */
TEST_F(DBInitLogic, TestInitMissingOne)
{
    std::filesystem::remove("/tmp/.cape/.cape.db");
    int res = hash_init_db((char *)home, strlen(home));
    EXPECT_EQ(res, -1); // Fails because one of the files is missing
}

/*
 * This test is testing for testing a file that is missing the magic byte
 */
TEST_F(DBInitLogic, TestMissingMagicBytes)
{
    std::filesystem::remove("/tmp/.cape/.cape.hash");
    std::ofstream output("/tmp/.cape/.cape.hash");
    int res = hash_init_db((char *)home, strlen(home));
    EXPECT_EQ(res, -1); // Identifies that the db file DOES NOT have the MAGIC
}

TEST_F(DBInitLogic, TestMissMatchDBHash)
{
    std::filesystem::remove("/tmp/.cape/.cape.db");
    std::ofstream output("/tmp/.cape/.cape.db");
    output << 0xFFAAFABA;
    int res = hash_init_db((char *)home, strlen(home));
    EXPECT_EQ(res, -1); // Identifies that the db file DOES NOT have the MAGIC

}

