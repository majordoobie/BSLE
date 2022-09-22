# Unit test the server arg function
add_executable(
        gtest_server_args
        gtest_server_args.cpp
)
target_link_libraries(
        gtest_server_args
        PUBLIC
        server_backend
)


# Unit test the server crypto functions
add_executable(
        gtest_server_crypto
        gtest_server_crypto.cpp
)
target_link_libraries(
        gtest_server_crypto
        PUBLIC
        server_crypto
)

# Unit test the server file api functions (it is contained within server_crypto)
add_executable(
        gtest_server_file_api
        gtest_server_file_api.cpp
)
target_link_libraries(
        gtest_server_file_api
        PUBLIC
        server_crypto
)



GTest_add_target(gtest_server_crypto)
GTest_add_target(gtest_server_file_api)
GTest_add_target(gtest_server_args)
