add_executable(
        gtest_server_args
        gtest_server_args.cpp
)
target_link_libraries(
        gtest_server_args
        PUBLIC
        server_backend
)

add_executable(
        gtest_server_crypto
        gtest_server_crypto.cpp
)

target_link_libraries(
        gtest_server_crypto
        PUBLIC
        server_crypto
)


GTest_add_target(gtest_server_crypto)
GTest_add_target(gtest_server_args)