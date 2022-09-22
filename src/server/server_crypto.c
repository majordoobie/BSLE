#include <openssl/sha.h>
#include <string.h>
#include <stdlib.h>

#include <server.h>
#include <stdbool.h>

DEBUG_STATIC void print_b_array(hash_t * array);
DEBUG_STATIC hash_t * hex_char_to_byte_array(const char *hash_str, size_t hash_size);

/*!
 * @brief Function takes a hash_t object and compares it against a hexadecimal
 * string that represents the hash to ensure they are the same.
 *
 * @param hash Pointer to a hash object to compare against
 * @param input Hexadecimal representation of the hash to compare
 * @param length Length of the hexadecimal representation
 * @return True if hashes match else false
 */
bool hash_pass_match(hash_t *hash, const char *input, size_t length)
{
    if ((NULL == hash) || (NULL == input))
    {
        return false;
    }

    hash_t * pw_hash = hex_char_to_byte_array(input, length);
    if (NULL == pw_hash)
    {
        return false;
    }

    for (size_t i = 0; i < pw_hash->size; i++)
    {
        if (pw_hash->array[i] != hash->array[i])
        {
            hash_destroy(& pw_hash);
            return false;
        }
    }
    hash_destroy(& pw_hash);
    return true;
}

/*!
 * @brief Hash the given byte array into a sha256 hash and return a hash_t
 * structure containing the byte array and its size
 *
 * @param input Pointer to the bytearray to hash
 * @param length Length of the bytearray to hash
 * @return hash_t object if successful or a NULL
 */
hash_t * hash_pass(const unsigned char * input, size_t length)
{
    if (NULL == input)
    {
        return NULL;
    }

    unsigned char md[SHA256_DIGEST_LENGTH];
    unsigned char * hash = SHA256((const unsigned char *)input, length, (unsigned char *)&md);
    if (NULL == hash)
    {
        fprintf(stderr, "[!] Unknown issue with SHA256\n");
        return NULL;
    }

    uint8_t * byte_array = (uint8_t *)calloc(SHA256_DIGEST_LENGTH, sizeof(uint8_t));
    if (UV_INVALID_ALLOC == verify_alloc(byte_array))
    {
        return NULL;
    }

    // The SHA256 hash is not a malloc pointer
    memcpy(byte_array, hash, SHA256_DIGEST_LENGTH);

    hash_t * array = (hash_t *)malloc(sizeof(hash_t));
    if (UV_INVALID_ALLOC == verify_alloc(array))
    {
        goto cleanup;
    }
    *array = (hash_t){
        .array = byte_array,
        .size = SHA256_DIGEST_LENGTH
    };
    return array;

cleanup:
    free(byte_array);
    return NULL;
}

/*!
 * @brief Free a hash_t object
 *
 * @param hash Pointer to the hash object to free
 */
void hash_destroy(hash_t ** hash)
{
    if (NULL == hash)
    {
        return;
    }
    hash_t * arr = *hash;
    if (NULL == arr)
    {
        return;
    }

    if (NULL != arr->array)
    {
        free(arr->array);
    }
    *arr = (hash_t){
        .array = NULL,
        .size = 0
    };
    free(arr);
    *hash = NULL;
}


DEBUG_STATIC void print_b_array(hash_t * array)
{
    if (NULL == array)
    {
        return;
    }
    for (size_t i = 0; i < array->size; i++)
    {
        printf("%02x", array->array[i]);
    }
    printf("\n");
}

/*!
 * @brief Translate a hexadecimal hash back into a byte array. Every two
 * characters of a hexadecimal array represents one byte. The function
 * reads two characters at a time and converts it to the byte representation.
 * The result is a byte array.
 *
 * @param hash_str Pointer to the hexadecimal string
 * @param hash_size Size of the hexadecimal string
 * @return hash_t object if successful otherwise NULL
 */
DEBUG_STATIC hash_t * hex_char_to_byte_array(const char *hash_str, size_t hash_size)
{
    if ((NULL == hash_str) || (0 == hash_size))
    {
        return NULL;
    }

    // The hash string represents a byte using two characters, so the byte
    // array must be half the size of the hash string
    if (0 != (hash_size % 2))
    {
        fprintf(stderr, "[!] The hash string provided contains an "
                        "odd number of hexadecimal characters.\n");
        return NULL;
    }

    size_t b_array_size = (size_t)(hash_size / 2);

    uint8_t * byte_array = (uint8_t *)calloc(b_array_size, sizeof(uint8_t));
    if (UV_INVALID_ALLOC == verify_alloc(byte_array))
    {
        return NULL;
    }

    int result;
    size_t str_idx  = 0;
    size_t barr_idx = 0;
    while (str_idx < hash_size)
    {
        result = sscanf(&hash_str[str_idx], "%2hhx", &byte_array[barr_idx]);

        // The scan API returns the number of bytes read. To check if a valid
        // read occurred a comparison of 1 byte is made.
        if (1 == result)
        {
            str_idx  += 2;
            barr_idx += 1;
        }
        // EOF is returned when either the end of the string is reached or
        // when an error occurred. Since we are reading with bounds, we should
        // only get EOF when an error occurs
        else if (EOF == result)
        {
            perror("sscanf");
            goto cleanup;
        }
    }

    hash_t * b_array = (hash_t *)malloc(sizeof(hash_t));
    if (UV_INVALID_ALLOC == verify_alloc(b_array))
    {
        goto cleanup;
    }
    *b_array = (hash_t){
        .array  = byte_array,
        .size   = b_array_size
    };
    return b_array;

cleanup:
    free(byte_array);
    return NULL;
}




