#include <gtest/gtest.h>
#include <server.h>

extern "C"
{
    byte_array_t * hex_char_to_byte_array(const char *hash_str, size_t hash_size);
    void print_b_array(byte_array_t * array);
}

TEST(T1, T1)
{
    char * string = strdup("password");
    byte_array_t * array = hash_pass(string, strlen(string));
    EXPECT_NE(nullptr, array);

    print_b_array(array);

    free(string);
    free_b_array(&array);
}