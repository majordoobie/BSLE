#include <server_crypto.h>

static const char * DB_DIR          = ".cape";
static const char * DB_NAME         = ".cape/.cape.db";
static const char * DB_HASH         = ".cape/.cape.hash";
static const char * DEFAULT_USER    = "admin";
static const char * DEFAULT_HASH    = "5e884898da28047151d0e56f8dc6292773603d0d6aabbdd62a11ef721d1542d8";
static const uint32_t MAGIC_BYTES   = 0xFFAAFABA;

static int init_db_dir(char * p_home_dir);
static verified_path_t * init_db_files(char * p_home_dir);
static verified_path_t * init_hash_file(char * p_home_dir, verified_path_t * p_db_file);

DEBUG_STATIC void print_b_array(hash_t * p_hash);
DEBUG_STATIC hash_t * hex_char_to_byte_array(const char * p_hash_str, size_t hash_size);

int8_t hash_init_db(char * p_home_dir, size_t dir_length)
{
    int result;
    // Check if the ${HOME_DIR}/.cape directory exists; If not, create it
    verified_path_t * p_path_hash = f_path_resolve(p_home_dir, DB_DIR);
    if (NULL == p_path_hash)
    {
        result = init_db_dir(p_home_dir);
        if (-1 == result)
        {
            goto ret_null;
        }
    }

    // Free the path, it is no longer needed now that we know the directory
    // exists we can now try to read the files within it
    if (NULL != p_path_hash)
    {
        free(p_path_hash);
    }


    // With the ${HOME_DIR}/.cape directory in existence, read the .cape.hash
    // and the .cape.db
    verified_path_t * p_hash_file = f_path_resolve(p_home_dir, DB_HASH);
    verified_path_t * p_db_file = f_path_resolve(p_home_dir, DB_NAME);
    if ((NULL == p_hash_file) && (NULL == p_db_file))
    {
        // Attempt to init the db_file
        p_db_file = init_db_files(p_home_dir);
        if (NULL == p_db_file)
        {
            goto ret_null;
        }
        p_hash_file = init_hash_file(p_home_dir, p_db_file);
        if (NULL == p_hash_file)
        {
            goto cleanup;
        }
    }

    printf("SUCCESS!\n");
    f_destroy_path(&p_db_file);
    f_destroy_path(&p_hash_file);
    return 0;

cleanup:
    f_destroy_path(&p_db_file);
ret_null:
    return -1;
}
static verified_path_t * init_hash_file(char * p_home_dir, verified_path_t * p_db_file)
{
    FILE * h_db_file = f_open_file(p_db_file, "r");
    if (NULL == h_db_file)
    {
        fprintf(stderr, "[!] Could not open the database file "
                        "in %s/%s you may have to create it yourself\n",
                DB_DIR, DB_NAME);
        goto ret_null;
    }

    // Get the files size to allocate a byte array
    fseek(h_db_file, 0L, SEEK_END);
    long int file_size = ftell(h_db_file);
    if (-1 == file_size)
    {
        perror("ftell");
        goto cleanup_file;
    }
    fseek(h_db_file, 0L, SEEK_SET);

    // Create the memory to read the files contents
    uint8_t * p_byte_array = (uint8_t *)calloc((unsigned long)file_size, sizeof(uint8_t));
    if (UV_INVALID_ALLOC == verify_alloc(p_byte_array))
    {
        goto cleanup_file;
    }

    // Read the contents of the file so that we can hash it
    size_t bytes_read = fread(p_byte_array, (unsigned long)file_size, sizeof(uint8_t), h_db_file);
    fclose(h_db_file);
    if (bytes_read != (unsigned long)file_size)
    {
        fprintf(stderr, "[!] Could not properly read the file\n");
        goto cleanup_array;
    }

    // Call the hashing function to get a hash structure to save the hash
    // string to the hash file
    hash_t * p_hash = hash_byte_array(p_byte_array, bytes_read);
    if (NULL == p_hash)
    {
        fprintf(stderr, "[!] Could not hash the file\n");
        goto cleanup_array;
    }

    // Get a verified path for the hash file to write the hash data to
    verified_path_t * p_hash_file = f_dir_resolve(p_home_dir, DB_HASH);
    if (NULL == p_hash_file)
    {
        fprintf(stderr, "[!] Could not create the database file "
                        "in %s/%s you may have to create it yourself\n",
                        DB_DIR, DB_NAME);
        goto cleanup_hash;
    }

    // Open a handle the hash file to write the contents to
    FILE * h_hash_file = f_open_file(p_hash_file, "w");
    if (NULL == h_hash_file) // Message printed already
    {
        goto cleanup_hash_file;
    }

    fwrite(&MAGIC_BYTES, sizeof(uint32_t), 1, h_hash_file);
    fwrite("\n", sizeof(char), 1, h_hash_file);
    fwrite(p_hash->array, sizeof(uint8_t), p_hash->size, h_hash_file);

    // clean up
    fclose(h_db_file);
    free(p_byte_array);
    hash_destroy(&p_hash);

    return p_hash_file;

cleanup_hash_file:
    f_destroy_path(&p_hash_file);
cleanup_hash:
    hash_destroy(&p_hash);
cleanup_array:
    free(p_byte_array);
cleanup_file:
    fclose(h_db_file);
ret_null:
    return NULL;
}

static verified_path_t * init_db_files(char * p_home_dir)
{
    // Attempt to resolve the path to the db file; This should never fail at
    // this point, but you can never be too sure
    verified_path_t * p_db = f_dir_resolve(p_home_dir, DB_NAME);
    if (NULL == p_db)
    {
        fprintf(stderr, "[!] Could not create the database file "
                        "in %s/%s you may have to create it yourself\n",
                        DB_DIR, DB_NAME);
        goto ret_null;
    }

    // Create a handle to the database file to write the defaults into it
    FILE * h_db = f_open_file(p_db, "w");
    if (NULL == h_db) // Message printed already
    {
        goto cleanup;
    }
    fwrite(&MAGIC_BYTES, sizeof(uint32_t), 1, h_db);
    fwrite("\n", sizeof(char), 1, h_db);
    fwrite(DEFAULT_USER, sizeof(char), strlen(DEFAULT_USER), h_db);
    fwrite(":", sizeof(char), 1, h_db);

    // Cool syntax uses a compound literal to create an unnamed object,
    // in this case an enum rvalue, then get its address
    fwrite(&(int){ADMIN}, sizeof(int), 1, h_db);

    fwrite(":", sizeof(char), 1, h_db);

    fwrite(DEFAULT_HASH, sizeof(char), strlen(DEFAULT_HASH), h_db);
    fwrite("\n", sizeof(char), 1, h_db);
    fclose(h_db);

    return p_db;

cleanup:
    f_destroy_path(&p_db);
ret_null:
    return NULL;
}

static int init_db_dir(char * p_home_dir)
{
    verified_path_t * p_db_dir = f_dir_resolve(p_home_dir, DB_DIR);
    if (NULL == p_db_dir)
    {
        return -1;
    }

    file_op_t status = f_create_dir(p_db_dir);
    if (FILE_OP_FAILURE == status)
    {
        fprintf(stderr, "[!] Could not create the database "
                        "directory in %s. You may have to perform this "
                        "operation yourself\n", DB_DIR);
        return -1;
    }

    return 0;
}

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




