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
        gtest_server_hash
        gtest_server_hash.cpp
)

target_link_libraries(
        gtest_server_hash
        PUBLIC
        server_hash
)


GTest_add_target(gtest_server_hash)
GTest_add_target(gtest_server_args)