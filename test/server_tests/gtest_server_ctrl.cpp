#include <gtest/gtest.h>
#include <server_ctrl.h>
#include <filesystem>
#include <fstream>


TEST(TestServerCTRL, TestDIR)
{
    const std::filesystem::path test_dir{"/tmp/CtrlTest"};
    std::filesystem::remove_all(test_dir);
    std::filesystem::create_directory(test_dir);

    verified_path_t * p_home_dir = f_set_home_dir(test_dir.c_str(), test_dir.string().size());
    db_t * p_db = db_init(p_home_dir);


    std::filesystem::remove_all(test_dir);
}