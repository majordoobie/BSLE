#include <server_db.h>

#define stringify(x) stringify2(x)
#define stringify2(x) #x

static const char * DB_DIR          = ".cape";
static const char * DB_NAME         = ".cape/.cape.db";
static const char * DB_HASH         = ".cape/.cape.hash";
static const char * DEFAULT_USER    = "admin";
static const char * DEFAULT_HASH    = "5e884898da28047151d0e56f8dc6292773603d0d6aabbdd62a11ef721d1542d8";
static const char * DB_FMT          = "%s:%hhd:%s\n";
//static const char * DB_SCAN_FMT     = "%s:%hhd:%s[^\n]";
static const uint32_t MAGIC_BYTES   = 0xFFAAFABA;

static verified_path_t * init_db_dir(char * p_home_dir);
static verified_path_t * init_db_file(const char * p_home_dir);
static verified_path_t * update_db_hash(const char * p_home_dir, verified_path_t * p_db_file);
static bool verify_magic(file_content_t * p_content);
static bool get_stored_hash(file_content_t * p_content);
static bool get_stored_data(file_content_t * p_content);
static int8_t populate_htable(htable_t * p_htable, file_content_t * p_contents);



// These functions are used for the creation of the hashtable that stores
// the user information
void free_key_callback(void * key);
void free_value_callback(void * payload);
static uint64_t hash_callback(void * key);
static htable_match_t compare_callback(void * left_key, void * right_key);

int8_t hash_init_db(char * p_home_dir, size_t dir_length)
{
    // Check if the ${HOME_DIR}/.cape directory exists; If not, create it
    verified_path_t * p_db_dir = f_path_resolve(p_home_dir, DB_DIR);
    if (NULL == p_db_dir)
    {
        debug_print("[!] Database dir %s/%s does not exist. Attempting to create"
                    " it.\n", p_home_dir, DB_DIR);
        p_db_dir = init_db_dir(p_home_dir);
        if (NULL == p_db_dir)
        {
            goto ret_null;
        }
    }
    // Once verified/created we no longer need the object
    f_destroy_path(&p_db_dir);

    // With the ${HOME_DIR}/.cape directory in existence, read the .cape.hash
    // and the .cape.db
    verified_path_t * p_hash_file = f_path_resolve(p_home_dir, DB_HASH);
    verified_path_t * p_db_file = f_path_resolve(p_home_dir, DB_NAME);
    if ((NULL == p_hash_file) && (NULL == p_db_file))
    {
        debug_print("%s\n", "[!] The database files do not exist, attempting to "
                    "create the defaults");
        // Attempt to init the db_file
        p_db_file = init_db_file(p_home_dir);
        if (NULL == p_db_file)
        {
            goto ret_null;
        }
        p_hash_file = update_db_hash(p_home_dir, p_db_file);
        if (NULL == p_hash_file)
        {
            goto cleanup_db;
        }
    }

    // If only one of the files exist, warn about the issue and exit
    else if ((NULL == p_hash_file) || (NULL == p_db_file))
    {
        const char * missing = (NULL == p_hash_file) ? DB_NAME : DB_HASH;
        const char * exist = (NULL != p_hash_file) ? DB_NAME : DB_HASH;

        fprintf(stderr, "[!] The \"%s\" file missing while \"%s\" "
                        "exist in the \"%s/%s\" home directory. Either return "
                        "the \"%s\" file or remove \"%s\" before starting "
                        "the server.\n",
                        missing, exist, p_home_dir, DB_DIR, missing, exist);
        goto cleanup_hash;
    }

    // Read the contents of the db file and the hash file
    file_content_t * p_db_contents = f_read_file(p_db_file);
    if (NULL == p_db_contents)
    {
        goto cleanup_hash;
    }
    file_content_t * p_hash_contents = f_read_file(p_hash_file);
    if (NULL == p_hash_contents)
    {
        goto cleanup_db_content;
    }

    f_destroy_path(&p_hash_file);
    f_destroy_path(&p_db_file);

    // Extract the hash for the .cape.db that is stored in the .cape.hash
    // The hash will replace the contents of p_hash_contents
    if (!get_stored_hash(p_hash_contents))
    {
        goto cleanup_hash_content;
    }

    // Check that the hash of the .cape.db matches the hash stored in .cape.hash
    // if it does not then return failure
    if (!hash_bytes_match(p_db_contents->p_hash,
                         p_hash_contents->p_stream,
                         p_hash_contents->stream_size))
    {
        fprintf(stderr, "[!] Hash stored does not match the hash "
                        "of the database. Revert the database base back to "
                        "what it was or remove all `.cape` files to start over.");
        goto cleanup_hash_content;
    }
    f_destroy_content(&p_hash_contents);


    // Check that the p_db_contents has the magic bytes and if it does
    // replace the contents array with the actual contents of the db
    if (!get_stored_data(p_db_contents))
    {
        goto cleanup_db_content;
    }


    // Create the hashtable object that is going to store the users
    htable_t * htable = htable_create(hash_callback, compare_callback,
                                      free_key_callback, free_value_callback);
    if (NULL == htable)
    {
        fprintf(stderr, "[!] Failed to create the hashtable"
                        "with the stored users\n");
        goto cleanup_hash_content;
    }

    populate_htable(htable, p_db_contents);



    htable_destroy(htable, HT_FREE_PTR_FALSE, HT_FREE_PTR_TRUE);
    f_destroy_content(&p_db_contents);
    return 0;

cleanup_hash_content:
    f_destroy_content(&p_hash_contents);
cleanup_db_content:
    f_destroy_content(&p_db_contents);
cleanup_hash:
    f_destroy_path(&p_hash_file);
cleanup_db:
    f_destroy_path(&p_db_file);
ret_null:
    return -1;
}


static int8_t populate_htable(htable_t * p_htable, file_content_t * p_contents)
{
    if (NULL == p_htable)
    {
        goto ret_null;
    }

    uint8_t perm;
    char username[MAX_USERNAME_LEN + 1];
    char pw_hash[(SHA256_DIGEST_LEN) + 1];
    hash_t * p_hash;
    char * p_username;



    int res = 0;
    char * segment = (char *)p_contents->p_stream;
    while (NULL != segment)
    {
        // Reset the copy variables for the next segment
        perm = 0;
        memset(username, 0, sizeof(username));
        memset(pw_hash, 0, sizeof(pw_hash));
        p_hash = NULL;
        p_username = NULL;

        // Find the username:perm:pw_hash from the segment and
        // populate the variables
        res = sscanf(segment,
                     "%" stringify(MAX_USERNAME_LEN) "[^:]:%hhu:%"
                     stringify(SHA256_DIGEST_LEN) "[^\n]",
                     username, & perm, pw_hash);

        // Verify that the tokens were parsed properly
        if (EOF == res)
        {
            break;
        }
        else if (res != 3)
        {
            fprintf(stderr, "[!] Invalid db format detected\n");
            goto ret_null;
        }

        // Convert the string hash to a hash_t object
        p_hash = hex_char_to_byte_array(pw_hash, strlen(pw_hash));
        if (NULL == p_hash)
        {
            fprintf(stderr, "[!] Unable to create memory for "
                            "pw_hash\n");
            goto ret_null;
        }

        // Create the username pointer
        p_username = strdup(username);
        if (UV_INVALID_ALLOC == verify_alloc(p_username))
        {
            goto cleanup_hash;
        }

        // Create the account object that will be saved into the hash table
        user_account_t * p_user = (user_account_t *)malloc(sizeof(user_account_t));
        if (UV_INVALID_ALLOC == verify_alloc(p_user))
        {
            goto cleanup_uname;
        }
        *p_user = (user_account_t){
            .p_username = p_username,
            .p_hash     = p_hash,
            .permission = perm
        };

        htable_set(p_htable, p_username, p_user);

        // Find the next segment with the new line feed and increment it by one
        segment = memchr(segment, '\n', p_contents->stream_size);
        segment = segment + 1;
    }
    return 0;

cleanup_uname:
    free(p_username);
cleanup_hash:
    hash_destroy(&p_hash);
ret_null:
    return -1;
}

/*!
 * @brief Read the data from the file_content_t object and verify that the
 * data has the magic bytes. If it does not return "false" indicating that
 * the file does not have the magic bytes. If successful, replace the read
 * bytes stream with the same stream but with the magic bytes absent.
 *
 * @param p_content Pointer a file_content_t object
 * @return True if the file_content_t object has the magic bytes and the
 * removal of the magic bytes from the file_content_t object
 */
static bool get_stored_data(file_content_t * p_content)
{
    if (!verify_magic(p_content))

    {
        goto ret_null;
    }

    size_t data_size = p_content->stream_size - sizeof(MAGIC_BYTES);

    // With the magic bytes verified, make room for the actual content
    uint8_t * p_stream = (uint8_t *)calloc(data_size,sizeof(uint8_t));
    if (UV_INVALID_ALLOC == verify_alloc(p_stream))
    {
        goto ret_null;
    }
    memcpy(p_stream, p_content->p_stream + sizeof(MAGIC_BYTES), data_size);
    free(p_content->p_stream);
    p_content->p_stream     = p_stream;
    p_content->stream_size  = data_size;
    return true;

ret_null:
    return false;
}

/*!
 * @brief Before sending the file content to get_stored_data, ensure that the
 * data stream stored in the file_content_t object has the length of the
 * SHA256 digest plus the magic bytes. If it does not, then do not bother
 * trying to extract the hash
 *
 * @param p_content Pointer to the file_content_t object
 * @return True if the file_content_t object has the magic bytes and the
 * removal of the magic bytes from the file_content_t object
 */
static bool get_stored_hash(file_content_t * p_content)
{
    if (NULL == p_content)
    {
        goto ret_null;
    }
    // Ensure that the data read has the same amount of bytes as the digest
    // and magic bytes
    if (p_content->stream_size != (SHA256_DIGEST_LENGTH + sizeof(MAGIC_BYTES)))
    {
        goto ret_null;
    }

    return get_stored_data(p_content);

ret_null:
    return false;
}

/*!
 * @brief Read the four bytes stored in the file_content_t that represents
 * the magic bytes.
 *
 * @param p_content Pointer to the file_content_t object
 * @return True if the file read has a matching magic byte else false
 */
static bool verify_magic(file_content_t * p_content)
{
    if (NULL == p_content)
    {
        goto ret_null;
    }
    if (p_content->stream_size < sizeof(MAGIC_BYTES))
    {
        goto ret_null;
    }

    uint32_t stored_magic = {0};
    memcpy(& stored_magic, p_content->p_stream, sizeof(MAGIC_BYTES));
    if (MAGIC_BYTES != stored_magic)
    {
        goto ret_null;
    }
    return true;

ret_null:
    return false;
}

/*!
 * @brief Update the .cape.hash file with the hash that represents the
 * .cape.db file. This is used to ensure that the file has not been corrupted
 *
 * @param p_home_dir Pointer to the servers home directory
 * @param p_db_file verified_path_t object to the .cape.db file
 * @return verified_path_t object representing the .cape.hash or NULL if error
 */
static verified_path_t * update_db_hash(const char * p_home_dir, verified_path_t * p_db_file)
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
    size_t bytes_read = fread(p_byte_array, sizeof(uint8_t),(unsigned long)file_size, h_db_file);
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
    verified_path_t * p_hash_file = f_valid_resolve(p_home_dir, DB_HASH);
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
    fwrite(p_hash->array, sizeof(uint8_t), p_hash->size, h_hash_file);

    // clean up
    fclose(h_hash_file);
    free(p_byte_array);
    hash_destroy(&p_hash);
    debug_print("%s\n", "[+] .cape.hash file updated with new .cape.db hash");

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

/*!
 * @brief Initialized the db file with the default admin user
 *
 * @param p_home_dir Pointer to the home directory of the server
 * @return If successful, a verified_path_t object is returned otherwise NULL
 */
static verified_path_t * init_db_file(const char * p_home_dir)
{
    // Attempt to resolve the path to the db file; This should never fail at
    // this point, but you can never be too sure
    verified_path_t * p_db = f_valid_resolve(p_home_dir, DB_NAME);
    if (NULL == p_db)
    {
        fprintf(stderr, "[!] Could not create the database file "
                        "in %s/%s you may have to create it yourself\n",
                p_home_dir, DB_NAME);
        goto ret_null;
    }

    // Create the p_buffer used to write to the file.
    // *NOTE* the + 4 is to account for the format and NULL ":", ":", "\n", "\0"
    size_t array_size = sizeof(MAGIC_BYTES) + strlen(DEFAULT_USER) + sizeof(uint8_t) + strlen(DEFAULT_HASH) + 4;
    uint8_t * p_buffer = (uint8_t *)calloc(array_size, sizeof(uint8_t));
    if (UV_INVALID_ALLOC == verify_alloc(p_buffer))
    {
        goto cleanup_path;
    }

    // Populate the p_buffer with the default data
    memcpy(p_buffer, &MAGIC_BYTES, 4);
    int write_bytes = sprintf((char *)(p_buffer + 4), DB_FMT, DEFAULT_USER, (uint8_t){ADMIN}, DEFAULT_HASH);
    //*NOTE* The - 4 is to exclude the magic bytes and the + 1 is to
    //include the "\0". sprintf does not consider this a byte written
    if ((array_size - 4) != (write_bytes + 1))
    {
        fprintf(stderr, "[!] Unable to properly write to db_file\n");
        goto cleanup_array;
    }

    file_op_t status = f_write_file(p_db, p_buffer, array_size);
    if (FILE_OP_FAILURE == status)
    {
        goto cleanup_array;
    }
    debug_print("%s\n", "[+] Successfully created the .cape.db file with "
                        "defaults");

    free(p_buffer);
    p_buffer = NULL;
    return p_db;

cleanup_array:
    free(p_buffer);
    p_buffer = NULL;
cleanup_path:
    f_destroy_path(&p_db);
ret_null:
    return NULL;
}

/*!
 * @brief Create the database directory to save the server information
 *
 * @param p_home_dir Pointer to the home directory of the server
 * @return verified_path_t object if successful, otherwise NULL
 */
static verified_path_t * init_db_dir(char * p_home_dir)
{
    verified_path_t * p_db_dir = f_valid_resolve(p_home_dir, DB_DIR);
    if (NULL == p_db_dir)
    {
        goto ret_null;
    }

    file_op_t status = f_create_dir(p_db_dir);
    if (FILE_OP_FAILURE == status)
    {
        fprintf(stderr, "[!] Could not create the database "
                        "directory in %s. You may have to perform this "
                        "operation yourself\n", DB_DIR);
        goto cleanup;
    }

    debug_print("%s\n", "[+] Successfully created the database directory");
    return p_db_dir;

cleanup:
    f_destroy_path(&p_db_dir);
ret_null:
    return NULL;
}

/*!
 * @brief Callback is used for the users_htable
 * @param key Pointer to the key object to be hashed. In this case, it is
 * the users username
 *
 * @return Hash value of the key
 */
static uint64_t hash_callback(void * key)
{
    char * s = (char *)key;
    uint64_t hash = htable_get_init_hash();

    htable_hash_key(&hash, s, strlen(s));
    return hash;
}

/*!
 * @brief Callback is used for the users_htable
 * @param left_key Left key to compare
 * @param right_key Right key to compare
 *
 * @return If the two keys match. In this case, if the two usernames match
 */
static htable_match_t compare_callback(void * left_key, void * right_key)
{
    char * left = (char *)left_key;
    char * right = (char *)right_key;

    // This test function only tests the payload strings. But you would compare
    // all items of the struct
    if (0 == strcmp(left, right))
    {
        return HT_MATCH_TRUE;
    }
    return HT_MATCH_FALSE;
}

/*!
 * @brief Callback is used for the user_htable
 *
 * @param payload Pointer to the value object stored in the hash table
 */
void free_value_callback(void * payload)
{
    if (NULL == payload)
    {
        return;
    }
    user_account_t * s = (user_account_t *)payload;
    if (NULL != s->p_username)
    {
        free(s->p_username);
    }
    if (NULL != s->p_hash)
    {
        hash_destroy(&s->p_hash);
    }

    *s = (user_account_t){
        .p_username = NULL,
        .p_hash     = NULL,
        .permission = 0,
    };
    free(s);
}

/*!
 * @brief Callback is used for the user_htable
 *
 * @param payload Pointer to the key object stored in the hash table
 */
void free_key_callback(void * key)
{
    char * s = (char *)key;
    free(s);
    s = NULL;
}
