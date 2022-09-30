#include <gtest/gtest.h>
#include <server_db.h>
#include <filesystem>
#include <fstream>
#include <server_ctrl.h>

static const std::filesystem::path test_dir{"/tmp/DBActions"};
static const std::filesystem::path file1{test_dir/"test_file1.txt"};
static bool init = true;

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
    wire_payload_t * payload1;
    wire_payload_t * payload2;
    wire_payload_t * payload3;
    wire_payload_t * payload4;

 protected:
    static void SetUpTestSuite()
    {
        std::filesystem::remove_all(test_dir);
        std::filesystem::create_directory(test_dir);

        std::ofstream output(file1);
        output << "Some data to write\nthe the file";
        output.close();


    }
    static void TearDownTestSuite()
    {
        std::filesystem::remove_all(test_dir);
    }

    DBUserActions()
    {
        // **NOTE** this value is not freed. It is a bit awkward here but the
        // server_args api will already create the verified_path_t in order
        // to verify that the path is valid
        p_home_dir = f_set_home_dir(test_dir.c_str(), test_dir.string().size());
        this->user_db = db_init(this->p_home_dir);
        if (NULL == this->user_db)
        {
            f_destroy_path(&p_home_dir);
            fprintf(stderr, "[!] Could not init\n");
            return;
        }

        if (init)
        {
            init = false;
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
                "New Belgium CO Denver", ADMIN);

            db_create_user(
                this->user_db,
                "Juicy Haze",
                "NB North Carolina", READ_WRITE);
        }
        this->user_db->_debug = true;

        user_payload_t * usr_payload1 = (user_payload_t *)calloc(1, sizeof(user_payload_t));
        usr_payload1->user_flag      = USR_ACT_CREATE_USER;
        usr_payload1->user_perm      = READ;
        usr_payload1->username_len   = strlen("VooDooRanger2");
        usr_payload1->p_username     = strdup("VooDooRanger2");
        usr_payload1->passwd_len     = strlen("New Belgium");
        usr_payload1->p_passwd       = strdup("New Belgium");

        this->payload1 = (wire_payload_t *)calloc(1, sizeof(wire_payload_t));
        this->payload1->opt_code       = ACT_USER_OPERATION;
        this->payload1->username_len   = strlen("VooDooRanger");
        this->payload1->passwd_len     = strlen("New Belgium");
        this->payload1->p_username     = strdup("VooDooRanger");
        this->payload1->p_passwd       = strdup("New Belgium");
        this->payload1->type           = USER_PAYLOAD;
        this->payload1->p_user_payload = usr_payload1;


        user_payload_t * usr_payload2 = (user_payload_t *)calloc(1, sizeof(user_payload_t));
        usr_payload2->user_flag      = USR_ACT_CREATE_USER;
        usr_payload2->user_perm      = READ;
        usr_payload2->username_len   = strlen("VooDooRanger");
        usr_payload2->p_username     = strdup("VooDooRanger");
        usr_payload2->passwd_len     = strlen("New Belgium");
        usr_payload2->p_passwd       = strdup("New Belgium");

        this->payload2 = (wire_payload_t *)calloc(1, sizeof(wire_payload_t));
        this->payload2->opt_code       = ACT_USER_OPERATION;
        this->payload2->username_len   = strlen("Fat Tire");
        this->payload2->passwd_len     = strlen("New Belgium CO Denver");
        this->payload2->p_username     = strdup("Fat Tire");
        this->payload2->p_passwd       = strdup("New Belgium CO Denver");
        this->payload2->type           = USER_PAYLOAD;
        this->payload2->p_user_payload = usr_payload2;


        std_payload_t * usr_payload3 = (std_payload_t *)calloc(1, sizeof(std_payload_t));
        usr_payload3->p_path = strdup(file1.filename().c_str());
        usr_payload3->path_len = strlen(file1.filename().c_str());

        std::ifstream infile(file1, std::ios::binary);
        infile.seekg(0, std::ios::end);
        size_t pos = (size_t)infile.tellg();
        infile.seekg(0, std::ios::beg);
        uint8_t * p_file = (uint8_t *)calloc(pos, sizeof(uint8_t));
        infile.read((char *)p_file, static_cast<long>(pos));
        infile.close();
        usr_payload3->p_byte_stream = p_file;
        usr_payload3->byte_stream_len = pos;

        this->payload3 = (wire_payload_t *)calloc(1, sizeof(wire_payload_t));
        this->payload3->opt_code       = ACT_LIST_REMOTE_DIRECTORY;
        this->payload3->username_len   = strlen("VooDooRanger");
        this->payload3->passwd_len     = strlen("New Belgium");
        this->payload3->p_username     = strdup("VooDooRanger");
        this->payload3->p_passwd       = strdup("New Belgium");
        this->payload3->type           = STD_PAYLOAD;
        this->payload3->p_std_payload  = usr_payload3;


        std_payload_t * usr_payload4 = (std_payload_t *)calloc(1, sizeof(std_payload_t));
        usr_payload4->p_path = strdup(file1.filename().c_str());
        usr_payload4->path_len = strlen(file1.filename().c_str());

        std::ifstream infile2(file1, std::ios::binary);
        infile2.seekg(0, std::ios::end);
        size_t pos2 = (size_t)infile2.tellg();
        infile2.seekg(0, std::ios::beg);
        uint8_t * p_file2 = (uint8_t *)calloc(pos2, sizeof(uint8_t));
        infile2.read((char *)p_file2, static_cast<long>(pos2));
        infile2.close();
        usr_payload4->p_byte_stream = p_file2;
        usr_payload4->byte_stream_len = pos2;

        this->payload4 = (wire_payload_t *)calloc(1, sizeof(wire_payload_t));
        this->payload4->opt_code       = ACT_LIST_REMOTE_DIRECTORY;
        this->payload4->username_len   = strlen("Juicy Haze");
        this->payload4->passwd_len     = strlen("NB North Carolina");
        this->payload4->p_username     = strdup("Juicy Haze");
        this->payload4->p_passwd       = strdup("NB North Carolina");
        this->payload4->type           = STD_PAYLOAD;
        this->payload4->p_std_payload  = usr_payload4;

    }
    ~DBUserActions()
    {
        db_shutdown(&this->user_db);
        ctrl_destroy(&this->payload1, NULL);
        ctrl_destroy(&this->payload2, NULL);
        ctrl_destroy(&this->payload3, NULL);
        ctrl_destroy(&this->payload4, NULL);
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
    free(this->payload1->p_passwd);
    this->payload1->p_passwd = strdup("Wrong Password");

    act_resp_t * resp = ctrl_parse_action(this->user_db, this->payload1);
    ASSERT_NE(resp, nullptr);
    EXPECT_EQ(resp->result, OP_USER_AUTH);
    ctrl_destroy(NULL, &resp);
}

TEST_F(DBUserActions, TestUserAction_CreateUserBadPerm)
{
    this->payload1->p_user_payload->user_perm = READ_WRITE;

    act_resp_t * resp = ctrl_parse_action(this->user_db, this->payload1);
    ASSERT_NE(resp, nullptr);
    EXPECT_EQ(resp->result, OP_PERMISSION_ERROR);
    ctrl_destroy(NULL, &resp);
}

TEST_F(DBUserActions, TestUserAction_CreateUserExists)
{
    free(this->payload1->p_user_payload->p_username);
    this->payload1->p_user_payload->p_username = strdup(this->payload1->p_username);

    act_resp_t * resp = ctrl_parse_action(this->user_db, this->payload1);
    ASSERT_NE(resp, nullptr);
    EXPECT_EQ(resp->result, OP_USER_EXISTS);
    ctrl_destroy(NULL, &resp);
}

TEST_F(DBUserActions, TestUserAction_DeleteUserNoPerm)
{
    this->payload1->p_user_payload->user_flag = USR_ACT_DELETE_USER;

    act_resp_t * resp = ctrl_parse_action(this->user_db, this->payload1);
    ASSERT_NE(resp, nullptr);
    EXPECT_EQ(resp->result, OP_PERMISSION_ERROR);
    ctrl_destroy(NULL, &resp);
}

TEST_F(DBUserActions, TestUserAction_DeleteUserNotExist)
{
    this->payload2->p_user_payload->user_flag = USR_ACT_DELETE_USER;
    free(this->payload2->p_user_payload->p_username);
    this->payload2->p_user_payload->p_username = strdup("UserNotExist");

    act_resp_t * resp = ctrl_parse_action(this->user_db, this->payload2);
    ASSERT_NE(resp, nullptr);
    EXPECT_EQ(resp->result, OP_USER_EXISTS);
    ctrl_destroy(NULL, &resp);
}

TEST_F(DBUserActions, TestUserAction_DeleteUser)
{
    this->payload2->p_user_payload->user_flag = USR_ACT_DELETE_USER;

    act_resp_t * resp = ctrl_parse_action(this->user_db, this->payload2);
    ASSERT_NE(resp, nullptr);
    EXPECT_EQ(resp->result, OP_SUCCESS);
    ctrl_destroy(NULL, &resp);
}

TEST_F(DBUserActions, TestUserAction_ListDirectoryErrorNotExist)
{
    this->payload3->opt_code = ACT_LIST_REMOTE_DIRECTORY;
    free(this->payload3->p_std_payload->p_path);
    this->payload3->p_std_payload->p_path = strdup("NotExist");
    this->payload3->p_std_payload->path_len = strlen("NotExist");

    act_resp_t * resp = ctrl_parse_action(this->user_db, this->payload3);
    ASSERT_NE(resp, nullptr);
    EXPECT_EQ(resp->result, OP_RESOLVE_ERROR);
    ctrl_destroy(NULL, &resp);
}

TEST_F(DBUserActions, TestUserAction_ListDirectoryError)
{
    this->payload3->opt_code = ACT_LIST_REMOTE_DIRECTORY;
    free(this->payload3->p_std_payload->p_path);
    this->payload3->p_std_payload->p_path = strdup(file1.filename().c_str());
    this->payload3->p_std_payload->path_len = strlen(file1.filename().c_str());

    act_resp_t * resp = ctrl_parse_action(this->user_db, this->payload3);
    ASSERT_NE(resp, nullptr);
    EXPECT_EQ(resp->result, OP_PATH_NOT_DIR);
    ctrl_destroy(NULL, &resp);
}

TEST_F(DBUserActions, TestUserAction_ListDirectory)
{
    this->payload3->opt_code = ACT_LIST_REMOTE_DIRECTORY;
    free(this->payload3->p_std_payload->p_path);
    this->payload3->p_std_payload->p_path = strdup("");
    this->payload3->p_std_payload->path_len = strlen("");

    act_resp_t * resp = ctrl_parse_action(this->user_db, this->payload3);
    ASSERT_NE(resp, nullptr);
    EXPECT_EQ(resp->result, OP_SUCCESS);
    printf("%.*s\n", (int)resp->p_content->stream_size, resp->p_content->p_stream);
    ctrl_destroy(NULL, &resp);
}

TEST_F(DBUserActions, TestUserAction_GetFile)
{
    this->payload3->opt_code = ACT_GET_REMOTE_FILE;

    act_resp_t * resp = ctrl_parse_action(this->user_db, this->payload3);
    ASSERT_NE(resp, nullptr);
    EXPECT_EQ(resp->result, OP_SUCCESS);
    printf("%.*s\n", (int)resp->p_content->stream_size, resp->p_content->p_stream);
    ctrl_destroy(NULL, &resp);
}

TEST_F(DBUserActions, TestUserAction_GetFileNotFile)
{
    this->payload3->opt_code = ACT_GET_REMOTE_FILE;
    free(this->payload3->p_std_payload->p_path);
    this->payload3->p_std_payload->p_path = strdup("");
    this->payload3->p_std_payload->path_len = strlen("");

    act_resp_t * resp = ctrl_parse_action(this->user_db, this->payload3);
    ASSERT_NE(resp, nullptr);
    EXPECT_EQ(resp->result, OP_PATH_NOT_FILE);
    ctrl_destroy(NULL, &resp);
}

TEST_F(DBUserActions, TestUserAction_GetFileError)
{
    this->payload3->opt_code = ACT_GET_REMOTE_FILE;
    free(this->payload3->p_std_payload->p_path);
    this->payload3->p_std_payload->p_path = strdup("not_exist");
    this->payload3->p_std_payload->path_len = strlen("not_exist");

    act_resp_t * resp = ctrl_parse_action(this->user_db, this->payload3);
    ASSERT_NE(resp, nullptr);
    EXPECT_EQ(resp->result, OP_RESOLVE_ERROR);
    ctrl_destroy(NULL, &resp);
}

TEST_F(DBUserActions, TestUserAction_PutFilePermError)
{
    this->payload3->opt_code = ACT_PUT_REMOTE_FILE;

    act_resp_t * resp = ctrl_parse_action(this->user_db, this->payload3);
    ASSERT_NE(resp, nullptr);
    EXPECT_EQ(resp->result, OP_PERMISSION_ERROR);
    ctrl_destroy(NULL, &resp);
}

TEST_F(DBUserActions, TestUserAction_PutFileFileExists)
{
    this->payload4->opt_code = ACT_PUT_REMOTE_FILE;

    act_resp_t * resp = ctrl_parse_action(this->user_db, this->payload4);
    ASSERT_NE(resp, nullptr);
    EXPECT_EQ(resp->result, OP_FILE_EXISTS);
    ctrl_destroy(NULL, &resp);
}

TEST_F(DBUserActions, TestUserAction_PutFile)
{
    this->payload4->opt_code = ACT_PUT_REMOTE_FILE;
    free(this->payload4->p_std_payload->p_path);
    this->payload4->p_std_payload->p_path = strdup("new_file.txt");
    this->payload4->p_std_payload->path_len = strlen("new_file.txt");

    act_resp_t * resp = ctrl_parse_action(this->user_db, this->payload4);
    ASSERT_NE(resp, nullptr);
    EXPECT_EQ(resp->result, OP_SUCCESS);
    ctrl_destroy(NULL, &resp);
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
