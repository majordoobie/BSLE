#include <gtest/gtest.h>
#include <server.h>

extern "C"
{
    byte_array_t * hex_char_to_byte_array(const char *hash_str, size_t hash_size);
    void print_b_array(byte_array_t * array);
}


 class ServerCryptoHashTest : public ::testing::TestWithParam<std::tuple<std::string, std::string>>{};

/*
 * I took the output of:
 * echo -n "password" | sha256sum
 * And pasted the output into "sha1sum" then converted that string into a
 * byte array and compared it to the hashing of "password" to ensure that
 * both outputs are the same
 */
TEST_P(ServerCryptoHashTest, TestValidPorts)
{
    auto [string_to_hash, sha256_sum_hash] = GetParam();


    byte_array_t * pw_hash = hash_pass(string_to_hash.c_str(), string_to_hash.size());
    EXPECT_NE(nullptr, pw_hash);

    byte_array_t * exp_hash = hex_char_to_byte_array(sha256_sum_hash.c_str(), sha256_sum_hash.size());
    EXPECT_NE(nullptr, exp_hash);

    EXPECT_EQ(pw_hash->size, exp_hash->size);

    for (size_t i = 0; i < pw_hash->size; i++)
    {
        EXPECT_EQ(pw_hash->array[i], exp_hash->array[i]);
    }

    free_b_array(&pw_hash);
    free_b_array(&exp_hash);
}

INSTANTIATE_TEST_SUITE_P(
    HashTests,
    ServerCryptoHashTest,
    ::testing::Values(
        std::make_tuple("DRAGON 1234 HHHHHHH", "3291e99245ab641a5693bf1f3701b9f961adf202d0e0b750ec89cd8338c4871a"),
        std::make_tuple("", "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"),
        std::make_tuple(" ", "36a9e7f1c95b82ffb99743e0c5c4ce95d83c9a430aac59f84ef3cbfab6145068"),
        std::make_tuple("hellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohel"
                        "lohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohe"
                        "llohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohe"
                        "llohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohe"
                        "llohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohel"
                        "lohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohel"
                        "lohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohel"
                        "lohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohell"
                        "ohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohello"
                        "hellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohe"
                        "llohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohell"
                        "ohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohelloh"
                        "ellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohel"
                        "lohellohellohellohellohello",
                        "bb3a5578b33ddbb427271a274aa0e5c2cc2934a63db71eab87855acfd7a1482c"),
        std::make_tuple("password", "5e884898da28047151d0e56f8dc6292773603d0d6aabbdd62a11ef721d1542d8")
    ));