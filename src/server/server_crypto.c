#include <server_crypto.h>

DEBUG_STATIC void print_b_array(hash_t * p_hash);
DEBUG_STATIC hash_t * hex_char_to_byte_array(const char * p_hash_str, size_t hash_size);
static bool hash_compare(uint8_t * l_array, uint8_t * r_array, size_t size);

/*!
 * @brief Function takes a hash_t object and compares it against a hexadecimal
 * string that represents the p_hash to ensure they are the same.
 *
 * @param p_hash Pointer to a p_hash object to compare against
 * @param p_input Hexadecimal representation of the p_hash to compare
 * @param length Length of the hexadecimal representation
 * @return True if hashes match else false
 */
bool hash_pass_match(hash_t * p_hash, const char * p_input, size_t length)
{
    if ((NULL == p_hash) || (NULL == p_input))
    {
        return false;
    }

    hash_t * p_pw_hash = hex_char_to_byte_array(p_input, length);
    if (NULL == p_pw_hash)
    {
        return false;
    }

    for (size_t i = 0; i < p_pw_hash->size; i++)
    {
        if (p_pw_hash->array[i] != p_hash->array[i])
        {
            hash_destroy(& p_pw_hash);
            return false;
        }
    }
    hash_destroy(&p_pw_hash);
    return true;
}

/*!
 * @brief Compare the hash of the hash_t to the array passed in
 *
 * @param p_hash Pointer to a p_hash object
 * @param p_array Pointer to an array representing the hash
 * @param array_size The size of the p_array
 * @return true if hashes match else false
 */
bool hash_bytes_match(hash_t * p_hash, uint8_t * p_array, size_t array_size)
{
    if ((NULL == p_hash) || (NULL == p_array))
    {
        goto ret_null;
    }

    if (p_hash->size != array_size)
    {
        goto ret_null;
    }

    return hash_compare(p_hash->array, p_array, array_size);

ret_null:
    return false;
}

/*!
 * @brief Compare if the hashes of two hash_t objects match
 *
 * @param p_lhash Pointer to a p_hash object
 * @param p_rhash Pointer to a p_hash object
 * @return true if hashes match else false
 */
bool hash_hash_t_match(hash_t * p_lhash, hash_t * p_rhash)
{
    if ((NULL == p_lhash) || (NULL == p_rhash))
    {
        goto ret_null;
    }

    if (p_rhash->size != p_lhash->size)
    {
        goto ret_null;
    }

    return hash_compare(p_lhash->array, p_rhash->array, p_lhash->size);


ret_null:
    return false;
}

/*!
 * @brief Compare if the two arrays match byte to byte
 * @param l_array Pointer to a byte array
 * @param r_array Pointer to a byte array
 * @param size Size of the byte arrays
 * @return true if arrays match else false
 */
static bool hash_compare(uint8_t * l_array, uint8_t * r_array, size_t size)
{
    for (size_t i = 0; i < size; i++)
    {
        if (l_array[i] != r_array[i])
        {
            return false;
        }
    }
    return true;
}

/*!
 * @brief Hash the given byte array into a sha256 hash and return a hash_t
 * structure containing the byte array and its size
 *
 * @param p_byte_array Pointer to the bytearray to hash
 * @param length Length of the bytearray to hash
 * @return hash_t object if successful or a NULL
 */
hash_t * hash_byte_array(uint8_t * p_byte_array, size_t length)
{
    if (NULL == p_byte_array)
    {
        return NULL;
    }

    unsigned char md[SHA256_DIGEST_LENGTH];
    uint8_t * hash_digest = (uint8_t *)SHA256((const unsigned char *)p_byte_array, length, (unsigned char *)&md);
    if (NULL == hash_digest)
    {
        fprintf(stderr, "[!] Unknown issue with SHA256\n");
        return NULL;
    }

    uint8_t * p_hash_digest = (uint8_t *)calloc(SHA256_DIGEST_LENGTH, sizeof(uint8_t));
    if (UV_INVALID_ALLOC == verify_alloc(p_hash_digest))
    {
        return NULL;
    }

    // The SHA256 hash_digest is not a malloc pointer
    memcpy(p_hash_digest, hash_digest, SHA256_DIGEST_LENGTH);

    hash_t * p_hash = (hash_t *)malloc(sizeof(hash_t));
    if (UV_INVALID_ALLOC == verify_alloc(p_hash))
    {
        goto cleanup;
    }
    *p_hash = (hash_t){
        .array = p_hash_digest,
        .size = SHA256_DIGEST_LENGTH
    };

    print_b_array(p_hash);
    return p_hash;

cleanup:
    free(p_hash_digest);
    return NULL;
}

/*!
 * @brief Free a hash_t object
 *
 * @param pp_hash Pointer to the pp_hash object to free
 */
void hash_destroy(hash_t ** pp_hash)
{
    if (NULL == pp_hash)
    {
        return;
    }
    hash_t * p_hash = *pp_hash;
    if (NULL == p_hash)
    {
        return;
    }

    if (NULL != p_hash->array)
    {
        free(p_hash->array);
    }
    *p_hash = (hash_t){
        .array = NULL,
        .size = 0
    };
    free(p_hash);
    *pp_hash = NULL;
}


DEBUG_STATIC void print_b_array(hash_t * p_hash)
{
    if (NULL == p_hash)
    {
        return;
    }
    for (size_t i = 0; i < p_hash->size; i++)
    {
        printf("%02x", p_hash->array[i]);
    }
    printf("\n");
}

/*!
 * @brief Translate a hexadecimal hash back into a byte array. Every two
 * characters of a hexadecimal array represents one byte. The function
 * reads two characters at a time and converts it to the byte representation.
 * The result is a byte array.
 *
 * @param p_hash_str Pointer to the hexadecimal string
 * @param hash_size Size of the hexadecimal string
 * @return hash_t object if successful otherwise NULL
 */
DEBUG_STATIC hash_t * hex_char_to_byte_array(const char * p_hash_str, size_t hash_size)
{
    if ((NULL == p_hash_str) || (0 == hash_size))
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

    size_t array_size = (size_t)(hash_size / 2);

    uint8_t * p_byte_array = (uint8_t *)calloc(array_size, sizeof(uint8_t));
    if (UV_INVALID_ALLOC == verify_alloc(p_byte_array))
    {
        return NULL;
    }

    int result;
    size_t str_idx  = 0;
    size_t barr_idx = 0;
    while (str_idx < hash_size)
    {
        result = sscanf(&p_hash_str[str_idx], "%2hhx", &p_byte_array[barr_idx]);

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

    hash_t * p_hash = (hash_t *)malloc(sizeof(hash_t));
    if (UV_INVALID_ALLOC == verify_alloc(p_hash))
    {
        goto cleanup;
    }
    *p_hash = (hash_t){
        .array  = p_byte_array,
        .size   = array_size
    };
    return p_hash;

cleanup:
    free(p_byte_array);
    return NULL;
}




