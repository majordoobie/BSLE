#include <gtest/gtest.h>
#include <server_db.h>
#include <filesystem>
#include <fstream>
#include <server_ctrl.h>

// Variables are used to synchronize the threads in ctest -j $(nproc)
static std::atomic_uint clear = 0;
static unsigned int tests = 6;

/*
 * These unit tests are so hard to synchronize with ctest -j since they
 * are running in parallel. The I/O tends to interfere with each other.
 * You will have to run the unit test twice if using ctest -j for it to work
 */
class DBUserActions : public ::testing::Test
{
 public:
    db_t * user_db;
    verified_path_t * p_home_dir;
    const std::filesystem::path test_dir{"/tmp/DBActions"};
 protected:
    void SetUp() override
    {
        bool init = false;
        if (!std::filesystem::exists(test_dir))
        {
            std::filesystem::create_directory(test_dir);
            init = true;
        }
        // **NOTE** this value is not freed. It is a bit awkward here but the
        // server_args api will already create the verified_path_t in order
        // to verify that the path is valid
        this->p_home_dir = f_set_home_dir(test_dir.c_str(), test_dir.string().size());
        this->user_db = db_init(this->p_home_dir);

        if (init)
        {
            db_create_user(
                this->user_db,
                "VooDooRanger",
                "New Belgium", READ);

            db_create_user(
                this->user_db,
                "VDooRanger Imperial",
                "New Belgium CO", READ);

            db_create_user(
                this->user_db,
                "Fat Tire",
                "New Belgium CO Denver", READ_WRITE);

            db_create_user(
                this->user_db,
                "Juicy Haze",
                "NB North Carolina", READ_WRITE);
        }

        this->user_db->_debug = true;

    }

    void TearDown() override
    {
        db_shutdown(&this->user_db);

        // Atomic function to clear the test directory. This is used to
        // synchronize the threads for ctest -j $(nrpoc)
        // note that we are evaluating tests - 1 because fetch_add returns the
        // previous value before it was incremented
        long long int val = clear.fetch_add(1);
        if (val >= (tests - 1))
        {
            fprintf(stderr, "[!] Cleaning up");
            remove_all(this->test_dir);
        }
    }
};

TEST_F(DBUserActions, TestUserExists)
{

    ret_codes_t res = db_create_user(
        this->user_db,
        "VooDooRanger",
        "New Belgium", READ);
    EXPECT_EQ(res, OP_USER_EXISTS);
}

TEST_F(DBUserActions, AuthSuccess)
{
    user_account_t * p_user = NULL;
    ret_codes_t res = db_authenticate_user(this->user_db, &p_user,
                                           "VooDooRanger",
                                           "New Belgium");
    EXPECT_EQ(res, OP_SUCCESS);
    EXPECT_NE(p_user, nullptr);
}

TEST_F(DBUserActions, AuthFailure)
{
    user_account_t * p_user = NULL;
    ret_codes_t res = db_authenticate_user(this->user_db, &p_user,
                                           "VooDooRanger",
                                           "New belgium");
    EXPECT_EQ(res, OP_USER_AUTH);
    EXPECT_EQ(p_user, nullptr);
}

TEST_F(DBUserActions, AuthLookupFailure)
{
    user_account_t * p_user = NULL;
    ret_codes_t res = db_authenticate_user(this->user_db, &p_user,
                                           "vooDooRanger",
                                           "New Belgium");
    EXPECT_EQ(res, OP_USER_AUTH);
    EXPECT_EQ(p_user, nullptr);
}

TEST_F(DBUserActions, FailureUserCreate)
{
    // Fails because p_username is less than 3 chars
    ret_codes_t res = db_create_user(
        this->user_db,
        "VD",
        "New Belgium CO", READ);
    EXPECT_EQ(res, OP_CRED_RULE_ERROR);

    // Fails because p_username is greater than 20 chars
    res = db_create_user(
        this->user_db,
        "VooDoo Ranger Juizy Haze",
        "New Belgium CO", READ);
    EXPECT_EQ(res, OP_CRED_RULE_ERROR);

    // Fails because password is less than 6 chars
    res = db_create_user(
        this->user_db,
        "VooDoo Ranger Juizy Haze",
        "Belg", READ);
    EXPECT_EQ(res, OP_CRED_RULE_ERROR);

    // Fails because password is greater than 30 chars
    res = db_create_user(
        this->user_db,
        "VooDoo Ranger Juizy Haze",
        "New Belgium - Denver Colorado USA", READ);
    EXPECT_EQ(res, OP_CRED_RULE_ERROR);
}

TEST_F(DBUserActions, UserDeletion)
{
    ret_codes_t res = db_remove_user(this->user_db, "VDooRanger Imperial");
    EXPECT_EQ(res, OP_SUCCESS);

    res = db_remove_user(this->user_db, "VDooRanger Imperial");
    EXPECT_EQ(res, OP_USER_EXISTS);
}

TEST_F(DBUserActions, TestUserAction_BadAuth)
{
    act_resp_t * resp = ctrl_parse_action(this->user_db, 0);
    ASSERT_NE(resp, nullptr);
    EXPECT_EQ(resp->result, OP_USER_AUTH);
    printf("%s\n", resp->msg);
    ctrl_destroy(&resp);
}



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
    db_t * htable = db_init(p_home_dir);
    EXPECT_NE(htable, nullptr); // Creates both
    db_shutdown(&htable);
    std::filesystem::remove_all(home);
}

db_t * reset_test(const char * path)
{
    verified_path_t * p_home_dir = f_set_home_dir(path, strlen(path));
    db_t * db = db_init(p_home_dir);
    if (NULL == db)
    {
        f_destroy_path(&p_home_dir);
    }
    return db;
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
    db_t * p_db = NULL;

    /*
     * Test the successful creation of the ./cape dir and the .cape/.cape.db and
     * .cape/.cape.hash
     */
    p_db = db_init(p_home_dir);
    EXPECT_NE(p_db, nullptr);
    db_shutdown(&p_db);
    std::filesystem::remove_all(home_cape);

    /*
     * Test that when one of the two mandatory files are missing that the
     * unit fails
     */
    p_db = reset_test(home);
    EXPECT_NE(p_db, nullptr);
    db_shutdown(&p_db);

    // Remove the db file to force a failure
    std::filesystem::remove(home_cape_db);
    p_db = reset_test(home);
    EXPECT_EQ(p_db, nullptr); // Fails because one of the files is missing
    std::filesystem::remove_all(home_cape);


    /*
     * Expect failure when the hash file does NOT have the magic bytes
     * making it an invalid file
     */
    p_db = reset_test(home);
    EXPECT_NE(p_db, nullptr);
    db_shutdown(&p_db);

    // Remove the contents of the hash file to force a failure of hash match
    std::filesystem::remove(home_cape_hash);
    std::ofstream output(home_cape_hash);
    p_db = reset_test(home);
    EXPECT_EQ(p_db, nullptr); // Identifies that the db file DOES NOT have the MAGIC
    std::filesystem::remove_all(home_cape);


    /*
     * Expect failure due to the hash of the .cape.db not matching the hash
     * in the .cape.hash
     */
    p_db = reset_test(home);
    EXPECT_NE(p_db, nullptr);
    db_shutdown(&p_db);

    std::filesystem::remove(home_cape_db);
    output << 0xFFAAFABA;
    p_db = reset_test(home);
    EXPECT_EQ(p_db, nullptr);

    db_shutdown(&p_db);
    std::filesystem::remove_all(home);
}
