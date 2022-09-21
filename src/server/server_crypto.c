#include <openssl/sha.h>
#include <string.h>
#include <stdlib.h>

#include <server.h>

DEBUG_STATIC void print_b_array(byte_array_t * array);
DEBUG_STATIC byte_array_t * hex_char_to_byte_array(const char *hash_str, size_t hash_size);

byte_array_t * hash_pass(const char *input, size_t length)
{
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

    byte_array_t * array = (byte_array_t *)malloc(sizeof(byte_array_t));
    if (UV_INVALID_ALLOC == verify_alloc(array))
    {
        goto cleanup;
    }
    *array = (byte_array_t){
        .array = byte_array,
        .size = SHA256_DIGEST_LENGTH
    };
    return array;

cleanup:
    free(byte_array);
    return NULL;
}

void free_b_array(byte_array_t ** array)
{
    if (NULL == array)
    {
        return;
    }
    byte_array_t * arr = *array;
    if (NULL == arr)
    {
        return;
    }

    if (NULL != arr->array)
    {
        free(arr->array);
    }
    *arr = (byte_array_t){
        .array = NULL,
        .size = 0
    };
    free(arr);
    *array = NULL;
}

DEBUG_STATIC void print_b_array(byte_array_t * array)
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

DEBUG_STATIC byte_array_t * hex_char_to_byte_array(const char *hash_str, size_t hash_size)
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

    byte_array_t * b_array = (byte_array_t *)malloc(sizeof(byte_array_t));
    if (UV_INVALID_ALLOC == verify_alloc(b_array))
    {
        goto cleanup;
    }
    *b_array = (byte_array_t){
        .array  = byte_array,
        .size   = b_array_size
    };
    return b_array;

cleanup:
    free(byte_array);
    return NULL;
}




