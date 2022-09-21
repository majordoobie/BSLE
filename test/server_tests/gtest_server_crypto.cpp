#include <gtest/gtest.h>
#include <server.h>

extern "C"
{
    byte_array_t * hex_char_to_byte_array(const char *hash_str, size_t hash_size);
    void print_b_array(byte_array_t * array);
}

/*
 * I took the output of:
 * echo -n "password" | sha256sum
 * And pasted the output into "sha1sum" then converted that string into a
 * byte array and compared it to the hashing of "password" to ensure that
 * both outputs are the same
 */
TEST(CompareHashTest, TestSha1Sum)
{
    char * string = strdup("password");
    const char * sha1sum = "5e884898da28047151d0e56f8dc6292773603d0d6aabbdd62a11ef721d1542d8";

    byte_array_t * pw_hash = hash_pass(string, strlen(string));
    EXPECT_NE(nullptr, pw_hash);

    byte_array_t * exp_hash = hex_char_to_byte_array(sha1sum, strlen(sha1sum));
    EXPECT_NE(nullptr, exp_hash);

    EXPECT_EQ(pw_hash->size, exp_hash->size);

    for (size_t i = 0; i < pw_hash->size; i++)
    {
        EXPECT_EQ(pw_hash->array[i], exp_hash->array[i]);
    }



    free(string);
    free_b_array(&pw_hash);
    free_b_array(&exp_hash);
}