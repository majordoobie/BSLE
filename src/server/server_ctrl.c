#include <server_ctrl.h>

static const char * DB_DIR          = ".cape";
static const char * DB_NAME         = ".cape/.cape.db";
static const char * DB_HASH         = ".cape/.cape.hash";
static const char * DEFAULT_USER    = "admin";
static const char * DEFAULT_HASH    = "5e884898da28047151d0e56f8dc6292773603d0d6aabbdd62a11ef721d1542d8";
static const uint32_t MAGIC_BYTES   = 0xFFAAFABA;

static int init_db_dir(char * p_home_dir);
static verified_path_t * init_db_files(char * p_home_dir);
static verified_path_t * init_hash_file(char * p_home_dir, verified_path_t * p_db_file);

int8_t hash_init_db(char * p_home_dir, size_t dir_length)
{
    int result;
    // Check if the ${HOME_DIR}/.cape directory exists; If not, create it
    verified_path_t * p_db_dir_path = f_path_resolve(p_home_dir, DB_DIR);
    if (NULL == p_db_dir_path)
    {
        result = init_db_dir(p_home_dir);
        if (-1 == result)
        {
            goto ret_null;
        }
    }

    // Free the path, it is no longer needed now that we know the directory
    // exists we can now try to read the files within it
    if (NULL != p_db_dir_path)
    {
        free(p_db_dir_path);
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
    fclose(h_hash_file);
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
//    FILE * h_db = f_open_file(p_db, "w");
//    if (NULL == h_db) // Message printed already
//    {
//        goto cleanup;
//    }

    size_t array_size = sizeof(MAGIC_BYTES) + strlen(DEFAULT_USER) + sizeof(int) + strlen(DEFAULT_HASH) + 3;
    uint8_t * buffer = (uint8_t *)calloc(array_size, sizeof(uint8_t));
    memcpy(buffer, &MAGIC_BYTES, 4);
    sprintf((char *)(buffer + 4), "%s:%d:%s\n", DEFAULT_USER, (int){ADMIN}, DEFAULT_HASH);
    f_write_file(p_db, buffer, array_size);

//    char buff[1024];
//    snprintf(buff, 1024, "%x\n%s:%d:%s\n", MAGIC_BYTES, DEFAULT_USER, (int){ADMIN}, DEFAULT_HASH);

//    fwrite(&MAGIC_BYTES, sizeof(uint32_t), 1, h_db);
//    fwrite("\n", sizeof(char), 1, h_db);
//    fwrite(buff, sizeof(char), strlen(buff), h_db);

//    fwrite(DEFAULT_USER, sizeof(char), strlen(DEFAULT_USER), h_db);
//    fwrite(":", sizeof(char), 1, h_db);
//
//    // Cool syntax uses a compound literal to create an unnamed object,
//    // in this case an enum rvalue, then get its address
//    fwrite(&(int){ADMIN}, sizeof(int), 1, h_db);
//
//    fwrite(":", sizeof(char), 1, h_db);
//
//    fwrite(DEFAULT_HASH, sizeof(char), strlen(DEFAULT_HASH), h_db);
//    fwrite("\n", sizeof(char), 1, h_db);
//    fclose(h_db);

    return p_db;

//cleanup:
//    f_destroy_path(&p_db);
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
