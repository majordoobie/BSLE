#include <gtest/gtest.h>
#include <server.h>

extern "C"
{
    hash_t * hex_char_to_byte_array(const char *hash_str, size_t hash_size);
    void print_b_array(hash_t * array);
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


    hash_t * pw_hash = hash_pass((const unsigned char *)string_to_hash.c_str(), string_to_hash.size());
    EXPECT_NE(nullptr, pw_hash);

    hash_t * exp_hash = hex_char_to_byte_array(sha256_sum_hash.c_str(), sha256_sum_hash.size());
    EXPECT_NE(nullptr, exp_hash);

    EXPECT_EQ(pw_hash->size, exp_hash->size);

    for (size_t i = 0; i < pw_hash->size; i++)
    {
        EXPECT_EQ(pw_hash->array[i], exp_hash->array[i]);
    }

    hash_destroy(& pw_hash);
    hash_destroy(& exp_hash);
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
        std::make_tuple("Number one. Steady hand. One day, Kim Jong Un need new heart. I do operation. But mistake! Kim Jong Un "
                        "die! SSD very mad! I hide fishing boat, come to America. No English, no food, no money. Darryl give me job. "
                        "Now I have house, American car and new woman. Darryl save life. Dude I own this NFT. Do you really think you "
                        "can get away with theft when you’re showing what you stole directly to my face. My lawyers will make an easy "
                        "job of this case. Prepare to say goodbye to your luscious life and start preparing for the streets. I will "
                        "ruin you. Noobmaster, hey it’s Thor again. You know, the god of thunder? Listen, buddy, if you don’t log off "
                        "this game immediately I will fly over to your house, and come down to that basement you’re hiding in and rip "
                        "off your arms and shove them up your butt! Oh, that’s right, yea just go cry to your father you little weasel."
                        "Number one. Steady hand. One day, Kim Jong Un need new heart. I do operation. But mistake! Kim Jong Un die! SSD "
                        "very mad! I hide fishing boat, come to America. No English, no food, no money. Darryl give me job. Now I have house, "
                        "American car and new woman. Darryl save life. Dude I own this NFT. Do you really think you can get away with theft "
                        "when you’re showing what you stole directly to my face. My lawyers will make an easy job of this case. Prepare to "
                        "say goodbye to your luscious life and start preparing for the streets. I will ruin you. Noobmaster, hey it’s Thor "
                        "again. You know, the god of thunder? Listen, buddy, if you don’t log off this game immediately I will fly over to "
                        "your house, and come down to that basement you’re hiding in and rip off your arms and shove them up your butt! Oh, "
                        "that’s right, yea just go cry to your father you little weasel.Number one. Steady hand. One day, Kim Jong Un need "
                        "new heart. I do operation. But mistake! Kim Jong Un die! SSD very mad! I hide fishing boat, come to America. No "
                        "English, no food, no money. Darryl give me job. Now I have house, American car and new woman. Darryl save life. "
                        "Dude I own this NFT. Do you really think you can get away with theft when you’re showing what you stole directly "
                        "to my face. My lawyers will make an easy job of this case. Prepare to say goodbye to your luscious life and start "
                        "preparing for the streets. I will ruin you. Noobmaster, hey it’s Thor again. You know, the god of thunder? Listen, "
                        "buddy, if you don’t log off this game immediately I will fly over to your house, and come down to that basement "
                        "you’re hiding in and rip off your arms and shove them up your butt! Oh, that’s right, yea just go cry to your father "
                        "you little weasel.",
                        "08c9c885c916f68371037a2ee92981b4b30d0b1d5a985393c8fcb255db2646ef"),
        std::make_tuple("password", "5e884898da28047151d0e56f8dc6292773603d0d6aabbdd62a11ef721d1542d8")
    ));


TEST(TestMatchingFunc, TestMatch)
{
    const char * hash_str = "5e884898da28047151d0e56f8dc6292773603d0d6aabbdd62a11ef721d1542d8";
    hash_t * hash = hash_pass((const unsigned char *)"password", 8);
    EXPECT_TRUE(hash_pass_match(hash, hash_str, strlen(hash_str)));
    hash_destroy(& hash);
}
