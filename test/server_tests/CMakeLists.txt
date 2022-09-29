# Unit test the server arg function
add_executable(
        gtest_server_args
        gtest_server_args.cpp
)
target_link_libraries(
        gtest_server_args
        PUBLIC
        server_ctrl
)


# Unit test the server crypto functions
add_executable(
        gtest_server_crypto
        gtest_server_crypto.cpp
)
target_link_libraries(
        gtest_server_crypto
        PUBLIC
        server_file_api
)

# Unit test the server file api functions (it is contained within server_file_api)
add_executable(
        gtest_server_file_api
        gtest_server_file_api.cpp
)
target_link_libraries(
        gtest_server_file_api
        PUBLIC
        server_file_api
)

# Unit test the server backend
add_executable(
        gtest_server_db
        gtest_server_db.cpp
)
target_link_libraries(
        gtest_server_db
        PUBLIC
        server_file_api
        server_ctrl
)

#add_executable(
#        gtest_server_ctrl
#        gtest_server_ctrl.cpp
#)
#target_link_libraries(
#        gtest_server_ctrl
#        PUBLIC
#        server_ctrl
#)
#
#
#GTest_add_target(gtest_server_ctrl)
GTest_add_target(gtest_server_db)
GTest_add_target(gtest_server_crypto)
GTest_add_target(gtest_server_file_api)
GTest_add_target(gtest_server_args)
