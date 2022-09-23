#include <gtest/gtest.h>
#include <server_ctrl.h>

TEST(TestDBInit, DBTest)
{
    const char * home = "/tmp";
    int res = hash_init_db((char *)home, strlen(home));
    EXPECT_EQ(res, 0);
}

