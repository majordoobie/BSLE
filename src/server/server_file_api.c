#include <server_file_api.h>

// Bytes needed to account for the "/" and a "\0"
#define SLASH_PLUS_NULL 2

// Files to ignore
extern const char * DB_DIR;
extern const char * DB_NAME;
extern const char * DB_HASH;

static uint8_t * realloc_buff(uint8_t * p_buffer,
                              size_t * p_size,
                              size_t offset);
static size_t get_file_size(verified_path_t * p_path, char name[256],
                            uint16_t * num_len);
DEBUG_STATIC char * join_and_resolve_paths(const char * p_root,
                                           size_t root_length,
                                           const char * p_child,
                                           size_t child_length);
static char * join_paths(const char * p_root, size_t root_length,
                         const char * p_child, size_t child_length);


// Simple structure is to ensure path paths passed to the API have already
// been validated
struct verified_path
{
    char * p_path;
};


/*!
 * @brief Access to the members of verified_path_t is private. But the need
 * to print the verified_path_t may be needed for debugging. This function
 * writes the path into the buffer provided.
 *
 * It is best to allocate the space needed with PATH_MAX
 *         char repr[PATH_MAX] = {0};
 *         f_path_repr(p_home_dir, repr, PATH_MAX)
 *
 * @param p_path Verified path object
 */
void f_path_repr(verified_path_t * p_path, char * path_repr, size_t path_size)
{
    if ((NULL == p_path)
        || (NULL == path_repr)
        || (path_size > PATH_MAX)
        || (strlen(p_path->p_path) > path_size))
    {
        return;
    }

    memset(path_repr, 0, path_size);
    for (size_t i = 0; i < strlen(p_path->p_path); i++)
    {
        path_repr[i] = p_path->p_path[i];
    }
}

/*!
 * @brief Function creates a verified path representing the home dir. A
 * verified path is a object that contains the path to path that exists
 * and exists with in the home dir.
 *
 * @param p_home_dir Pointer to the home dir string
 * @param dir_size Size of the home dir string
 * @return Verified path if path exists else NULL
 */
verified_path_t * f_set_home_dir(const char * p_home_dir, size_t dir_size)
{
    if ((NULL == p_home_dir))
    {
        goto ret_null;
    }

    char * p_join_path = join_and_resolve_paths(p_home_dir, dir_size,
                                                "", 0);
    if (NULL == p_join_path)
    {
        goto ret_null;
    }

    // Finally, create the verified_path_t object and set the path to the
    // verified path that "can" exist
    verified_path_t * p_path = (verified_path_t *)malloc(sizeof(verified_path_t));
    if (UV_INVALID_ALLOC == verify_alloc(p_path))
    {
        fprintf(stderr, "[!] Homedir path provided did not resolve\n");
        goto cleanup;
    }

    p_path->p_path = p_join_path;
    return p_path;

cleanup:
    free(p_join_path);
ret_null:
    return NULL;
}

/*!
 * @brief Exact same function as f_valid_resolve except that a verified path
 * of the homedir is used.
 *
 * @param p_home_dir verified_path_t object of the home dir
 * @param p_child Pointer to the child path to resolve
 * @return Verified path if the path is valid or else NULL
 */
verified_path_t * f_ver_valid_resolve(verified_path_t * p_home_dir,
                                      const char * p_child)
{
    if ((NULL == p_home_dir) || (NULL == p_child))
    {
        goto ret_null;
    }
    return f_valid_resolve(p_home_dir->p_path, p_child);

ret_null:
    return NULL;
}

/*!
 * @brief Exact same function as f_path_resolve except that a verified path
 * of the homedir is used.
 *
 * @param p_home_dir verified_path_t object of the home dir
 * @param p_child Pointer to the child path to resolve
 * @return Verified path if the path is valid or else NULL
 */
verified_path_t * f_ver_path_resolve(verified_path_t * p_home_dir,
                                     const char * p_child)
{
    if ((NULL == p_home_dir) || (NULL == p_child))
    {
        goto ret_null;
    }
    return f_path_resolve(p_home_dir->p_path, p_child);

ret_null:
    return NULL;

}

/*!
 * @brief Just like `f_path_resolve`, the function checks to ensure that the
 * path created is a valid path. The only difference is that this function
 * checks if the final path joined can possibly be created. This is done
 * by ensuring that the path follows filename rules and the file that will
 * be created will exist within the home directory.
 *
 * @param p_home_dir Pointer to the home directory path
 * @param p_child Pointer to the child path to resolve to
 * @return verified_path_t object or a NULL is returned if the file path
 * character limit is exceeded, if the file does not exist or if the file exists but outside the home directory.
 */
verified_path_t * f_valid_resolve(const char * p_home_dir, const char * p_child)
{
    if ((NULL == p_home_dir) || (NULL == p_child))
    {
        goto ret_null;
    }

    // Make a copy of the child path since dirname function modifies the str
    char * p_child_cpy = strdup(p_child);
    if (UV_INVALID_ALLOC == verify_alloc(p_child_cpy))
    {
        goto ret_null;
    }

    // Attempt to resolve a path within the home dir using the child paths
    // dir path
    char * child_dir = dirname(p_child_cpy);
    size_t home_dir_len = strlen(p_home_dir);
    char * p_join_path = join_and_resolve_paths(p_home_dir,
                                                home_dir_len,
                                                child_dir,
                                                strlen(child_dir));
    if (NULL == p_join_path)
    {
        goto cleanup_cpy;
    }

    // Now that we know that the dirname of the child path exists within
    // the home directory, make a new child copy to call the dirbase name
    // function which modifies the passed in pointer
    free(p_child_cpy);
    p_child_cpy = NULL;
    p_child_cpy = strdup(p_child);
    if (UV_INVALID_ALLOC == verify_alloc(p_child_cpy))
    {
        goto cleanup_join;
    }

    // With the verified directory path and the file name, join the two paths
    // together to create a "valid" new file that can exist within the home dir
    char * child_basename = basename(p_child_cpy);
    // A child_dir of "." is a path with no parent. Therefore, the file path
    // passed in is empty
    if ((1 == strlen(child_basename)) && ('.' == child_basename[0]))
    {
        goto cleanup_join;
    }
    char * p_final_path = join_paths(p_join_path,
                                     strlen(p_join_path),
                                     child_basename,
                                     strlen(child_basename));
    if (NULL == p_final_path)
    {
        goto cleanup_join;
    }

    if (0 != strncmp(p_home_dir, p_final_path, home_dir_len))
    {
        fprintf(stderr, "[!] File path provided does not exist "
                        "within the home directory of the server\n->"
                        "[DIR] %s\n->[FILE]%s\n", p_home_dir, p_final_path);
        goto cleanup_rsvl;
    }

    // Finally, create the verified_path_t object and set the path to the
    // verified path that "can" exist
    verified_path_t
        * p_path = (verified_path_t *)malloc(sizeof(verified_path_t));
    if (UV_INVALID_ALLOC == verify_alloc(p_path))
    {
        goto cleanup_rsvl;
    }

    p_path->p_path = p_final_path;
    free(p_join_path);
    free(p_child_cpy);
    return p_path;

cleanup_rsvl:
    free(p_final_path);
cleanup_join:
    free(p_join_path);
cleanup_cpy:
    if (NULL != p_child_cpy)
    {
        free(p_child_cpy);
    }
ret_null:
    return NULL;
}

/*!
 * @brief Function verifies that the combination of the joining of the parent
 * and child paths do not exceed the path character limit.
 *
 * The two paths are then joined together and verified if the path exists.
 * The resolved path is then checked to see if the path exists within the
 * home directory of the server.
 *
 * @param p_home_dir Pointer to the home directory path
 * @param p_child Pointer to the child path to resolve to
 * @return verified_path_t object or a NULL is returned if the file path
 * character limit is exceeded, if the file does not exist or if the file exists but outside the home directory.
 */
verified_path_t * f_path_resolve(const char * p_home_dir, const char * p_child)
{
    if ((NULL == p_home_dir) || (NULL == p_child))
    {
        goto ret_null;
    }

    size_t home_dir_len = strlen(p_home_dir);
    size_t child_len = strlen(p_child);

    if ((home_dir_len + child_len + SLASH_PLUS_NULL) > PATH_MAX)
    {
        fprintf(stderr, "[!] Resolve path exceeds the file path "
                        "character limit\n");
        goto ret_null;
    }

    char * p_join_path =
        join_and_resolve_paths(p_home_dir, home_dir_len, p_child, child_len);
    if (NULL == p_join_path)
    {
        goto ret_null;
    }

    if (0 != strncmp(p_home_dir, p_join_path, home_dir_len))
    {
        fprintf(stderr, "[!] File path provided does not exist "
                        "within the home directory of the server\n->"
                        "[DIR] %s\n->[FILE]%s\n", p_home_dir, p_join_path);
        goto cleanup;
    }

    verified_path_t
        * p_path = (verified_path_t *)malloc(sizeof(verified_path_t));
    if (UV_INVALID_ALLOC == verify_alloc(p_path))
    {
        goto cleanup;
    }

    p_path->p_path = p_join_path;
    return p_path;

cleanup:
    free(p_join_path);
ret_null:
    return NULL;
}

/*!
 * @brief Destroy the verified_path_t object
 *
 * @param pp_path Double pointer to the verified_path_t object
 */
void f_destroy_path(verified_path_t ** pp_path)
{
    if (NULL == pp_path)
    {
        return;
    }
    verified_path_t * p_path = * pp_path;
    if (NULL == p_path)
    {
        return;
    }

    if (NULL != p_path->p_path)
    {
        free(p_path->p_path);
        p_path->p_path = NULL;
    }
    free(p_path);
    * pp_path = NULL;
}

/*!
 * @brief Simple wrapper for creating a directory using the verified_path_t
 * object.
 *
 * Usage:
 *  verified_path_t * p_db_dir = f_ver_valid_resolve(p_home_dir, path);
 *  ret_codes_t status = f_create_dir(p_db_dir);
 *
 *
 * @param p_path Pointer to a verified_path_t object
 * @retval OP_SUCCESS When the directory is created
 * @retval OP_FAILURE When there is a creation error
 */
ret_codes_t f_create_dir(verified_path_t * p_path)
{
    if ((NULL == p_path) || (NULL == p_path->p_path))
    {
        goto ret_null;
    }

    int result = mkdir(p_path->p_path, 0777);
    if (-1 == result)
    {
        perror("mkdir");
        goto ret_null;
    }
    return OP_SUCCESS;

ret_null:
    return OP_FAILURE;
}

/*!
 * @brief Delete the file. If the file is of type directory, first check
 * the the directory is empty. If it is empty, attempt to delete the directory.
 *
 * @param p_path Pointer to a verified_path_t object
 * @retval OP_SUCCESS Successfully removed the file/directory
 * @retval OP_DIR_NOT_EMPTY When attempting to delete a directory that is not
 * empty
 * @retval OP_FAILURE Server errors
 */
ret_codes_t f_del_file(verified_path_t * p_path)
{
    if (NULL == p_path)
    {
        goto ret_null;
    }

    struct stat stat_buff = {0};
    if (-1 == stat(p_path->p_path, &stat_buff))
    {
        debug_print_err("[!] Unable to get stats for %s\n:Error: %s\n",
                        p_path->p_path, strerror(errno));
        goto ret_null;
    }

    // Check if the file path is a file
    if (S_ISREG(stat_buff.st_mode))
    {
        if (-1 == unlink(p_path->p_path))
        {
            debug_print_err("[!] Unable to unlink %s\n:Error: %s\n",
                            p_path->p_path, strerror(errno));
            goto ret_null;
        }
    }
    else if (S_ISDIR(stat_buff.st_mode))
    {
        // File count is used to count the number of files in the directory
        // if the directory has only two files ("." and "..") then the directory
        // is empty
        uint8_t file_count = 0;
        struct dirent * obj;
        DIR * h_dir = opendir(p_path->p_path);
        obj = readdir(h_dir);
        while ((NULL != obj) && (file_count < 4))
        {
            file_count++;
            obj = readdir(h_dir);
        }
        closedir(h_dir);
        if (file_count > 2)
        {
            goto dir_not_empty;
        }
        else
        {
            if (-1 == rmdir(p_path->p_path))
            {
                debug_print_err("[!] Unable to remove directory %s\n:Error: %s\n",
                                p_path->p_path, strerror(errno));
                goto ret_null;
            }
        }
    }
    else
    {
        debug_print_err("[!] File %s is not a regular file or directory\n",
                        p_path->p_path);
        goto ret_null;
    }

    return OP_SUCCESS;

dir_not_empty:
    return OP_DIR_NOT_EMPTY;
ret_null:
    return OP_FAILURE;
}

/*!
 * @brief Simple wrapper for opening the verified_path_t object
 *
 * @param p_path Pointer to the verified_path_t object
 * @param p_read_mode Mode to open the verified_path_t object with
 * @return File object or NULL if the path cannot be opened
 */
FILE * f_open_file(verified_path_t * p_path, const char * p_read_mode)
{
    if ((NULL == p_path) || (NULL == p_read_mode) || (NULL == p_path->p_path))
    {
        goto ret_null;
    }

    FILE * h_file = fopen(p_path->p_path, p_read_mode);
    if (NULL == h_file)
    {
        perror("fopen");
        goto ret_null;
    }

    return h_file;

ret_null:
    return NULL;
}

/*!
 * @brief Simple wrapper to write the data stream to the verified file path
 *
 * @param p_path Pointer to a verified_file_t object
 * @param p_stream Pointer to a byte stream
 * @param stream_size Number of bytes in the byte stream
 * @return FILE_OP_SUCCESS if operation succeeded, otherwise FILE_OP_FAILURE
 */
ret_codes_t f_write_file(verified_path_t * p_path,
                         uint8_t * p_stream,
                         size_t stream_size)
{
    if ((NULL == p_path) || (NULL == p_stream))
    {
        goto ret_null;
    }

    FILE * h_path = f_open_file(p_path, "w");
    if (NULL == h_path) // Message printed already
    {
        goto ret_null;
    }

    size_t write = fwrite(p_stream, sizeof(uint8_t), stream_size, h_path);
    if (write != stream_size)
    {
        fprintf(stderr, "[!] Unable to write all bytes to %s\n",
                p_path->p_path);
        goto cleanup;
    }

    fclose(h_path);
    return OP_SUCCESS;

cleanup:
    fclose(h_path);
ret_null:
    return OP_FAILURE;
}

/*!
 * @brief Read wrapper is used to read the verified file path. If successful,
 * the data read is hashed and all the metadata about the stream is added
 * into the file_content_t object.
 *
 * @param p_path Pointer to a verified_path_t object
 * @return file_content_t object if successful, otherwise NULL
 */
file_content_t * f_read_file(verified_path_t * p_path, ret_codes_t * p_code)
{
    *p_code = OP_IO_ERROR;
    if (NULL == p_path)
    {
        goto ret_null;
    }

    struct stat stat_buff = {0};
    if (-1 == stat(p_path->p_path, &stat_buff))
    {
        debug_print_err("[!] Unable to get stats for %s\n:Error: %s\n",
                        p_path->p_path, strerror(errno));
        goto ret_null;
    }

    if (!S_ISREG(stat_buff.st_mode))
    {
        fprintf(stderr, "[!] Path %s given is not a regular file\n",
                p_path->p_path);
        *p_code = OP_PATH_NOT_FILE;
        goto ret_null;
    }

    FILE * h_path = f_open_file(p_path, "r");
    if (NULL == h_path)
    {
        fprintf(stderr, "[!] Could not open the %s file for "
                        "reading\n", p_path->p_path);
        goto ret_null;
    }

    // Get the files size to allocate a byte array
    int result = fseek(h_path, 0L, SEEK_END);
    if (-1 == result)
    {
        fprintf(stderr, "[!] Error attempting to seek file %s: "
                        "%s", p_path->p_path, strerror(errno));
        goto cleanup_close;
    }
    long int file_size = ftell(h_path);
    if (-1 == file_size)
    {
        fprintf(stderr, "[!] Error attempting to ftell file %s: "
                        "%s", p_path->p_path, strerror(errno));
        goto cleanup_close;
    }
    result = fseek(h_path, 0L, SEEK_SET);
    if (-1 == result)
    {
        fprintf(stderr, "[!] Error attempting to seek file %s: "
                        "%s", p_path->p_path, strerror(errno));
        goto cleanup_close;
    }

    // Create the byte array to read the contents of the file
    uint8_t * p_byte_array =
        (uint8_t *)calloc((unsigned long)file_size, sizeof(uint8_t));
    if (UV_INVALID_ALLOC == verify_alloc(p_byte_array))
    {
        *p_code = OP_FAILURE;
        goto cleanup_close;
    }

    // Read the contents into the p_byte_array created
    size_t bytes_read = fread(p_byte_array, sizeof(uint8_t), (unsigned long)file_size, h_path);
    fclose(h_path);
    if (bytes_read != (unsigned long)file_size)
    {
        fprintf(stderr, "[!] Unable to read all the bytes from the "
                        "file %s\n", p_path->p_path);
        goto cleanup_array;
    }

    // Hash the data stream
    hash_t * p_hash = hash_byte_array(p_byte_array, bytes_read);
    if (NULL == p_hash)
    {
        fprintf(stderr, "[!] Unable to hash the contents of "
                        "%s", p_path->p_path);
        *p_code = OP_FAILURE;
        goto cleanup_array;
    }

    // Create the file read content
    file_content_t
        * p_content = (file_content_t *)malloc(sizeof(file_content_t));
    if (UV_INVALID_ALLOC == verify_alloc(p_content))
    {
        *p_code = OP_FAILURE;
        goto cleanup_hash;
    }

    // Duplicate the file path, the verified path is not needed inside the
    // contents structure
    char * p_file_path = strdup(p_path->p_path);
    if (UV_INVALID_ALLOC == verify_alloc(p_file_path))
    {
        *p_code = OP_FAILURE;
        goto cleanup_content;
    }

    // Save data into the content structure to return
    * p_content = (file_content_t){
        .p_stream       = p_byte_array,
        .p_hash         = p_hash,
        .stream_size    = bytes_read,
        .p_path         = p_file_path
    };

    *p_code = OP_SUCCESS;
    return p_content;

cleanup_content:
    free(p_content); // Content is not populated here so no destroy is called
cleanup_hash:
    hash_destroy(& p_hash);
cleanup_array:
    free(p_byte_array);
cleanup_close:
    fclose(h_path);
ret_null:
    return NULL;
}

/*!
 * @brief Iterate over all the files in the dir path provided and create
 * a byte array with the file type [F] for file or [D] for dir along with
 * the file size and file name.
 *
 * The data is stitched together using f_type:f_size:f_name\n
 *
 * @param p_path  Pointer to the path to list
 * @return A file content containing the array of data to return or NULL if
 * a failure occurred
 */
file_content_t * f_list_dir(verified_path_t * p_path, ret_codes_t * p_code)
{
    *p_code = OP_IO_ERROR;
    if (NULL == p_path)
    {
        goto ret_null;
    }

    uint8_t * p_buffer = NULL;
    struct stat stat_buff = {0};
    if (-1 == stat(p_path->p_path, &stat_buff))
    {
        debug_print_err("[!] Unable to get stats for %s\n:Error: %s\n",
                        p_path->p_path, strerror(errno));
        goto ret_null;
    }

    if (!S_ISDIR(stat_buff.st_mode))
    {
        fprintf(stderr, "[!] Path %s given is not a directory\n",
                p_path->p_path);
        *p_code = OP_PATH_NOT_DIR;
        goto ret_null;
    }


    // Allocate a p_buffer that we can resize for the string output
    size_t buff_size = 1024;
    size_t offset = 0;
    int writes = 0;
    p_buffer = (uint8_t *)calloc(buff_size, sizeof(uint8_t));
    if (UV_INVALID_ALLOC == verify_alloc(p_buffer))
    {
        *p_code = OP_FAILURE;
        goto ret_null;
    }


    struct dirent * obj;
    DIR * h_dir = opendir(p_path->p_path);
    if (NULL == h_dir)
    {
        fprintf(stderr, "[!] Could not open %s\nError: %s\n", p_path->p_path,
                strerror(errno));
        goto cleanup_buff;
    }

    // Iterate over the path given looking for:
    // -> File types: dir or regular
    // -> File names not "." or ".."
    obj = readdir(h_dir);
    while (NULL != obj)
    {
        // obj->d_name is guaranteed to have a '\0'
        if (((DT_REG == obj->d_type)
                || (DT_DIR == obj->d_type))
                && ((0 != strcmp(obj->d_name, ".")
                     && (0 != strcmp(obj->d_name, "..")))
                     && (0 != strcmp(obj->d_name, DB_DIR))
                     && (0 != strcmp(obj->d_name, DB_HASH))
                     && (0 != strcmp(obj->d_name, DB_NAME))
                     ))
        {
            // Ignore the `.cape` directory

            // Get the file size and the length of digits to represent the length
            uint16_t num_length = 0;
            size_t file_size = get_file_size(p_path, obj->d_name, &num_length);

            // Check if the buffer has enough room to write the string
            // The 4 represents ":", ":", "\n", "\0"
            if ((strlen(obj->d_name) + offset + num_length) + 4 >= buff_size)
            {
                // realloc_buff will free the buffer on failure
                p_buffer = realloc_buff(p_buffer, &buff_size, offset);
                if (NULL == p_buffer)
                {
                    *p_code = OP_FAILURE;
                    goto ret_null;
                }
            }
            writes = sprintf((char *)(p_buffer + offset), "[%s]:%ld:%s\n",
                             (obj->d_type == DT_REG ? "F" : "D"),
                             file_size,
                             obj->d_name);
            if (writes < 0)
            {
                fprintf(stderr, "[!] Write error reading directory\n");
                goto cleanup_buff;
            }
            offset += (size_t)writes;
        }
        obj = readdir(h_dir);
    }
    closedir(h_dir);

    // Create the buffer that is going to be returned
    size_t str_len = strlen((char *)p_buffer);
    uint8_t * p_buff = (uint8_t *)calloc(str_len, sizeof(uint8_t));
    if (UV_INVALID_ALLOC == verify_alloc(p_buff))
    {
        *p_code = OP_FAILURE;
        goto cleanup_buff;
    }

    // Copy the data to the new memory block. This is what will be used
    // from now on.
    memcpy(p_buff, p_buffer, str_len);

    // Hash the data stream
    hash_t * p_hash = hash_byte_array(p_buff, str_len);
    if (NULL == p_hash)
    {
        fprintf(stderr, "[!] Unable to hash the contents of "
                        "%s", p_path->p_path);
        *p_code = OP_FAILURE;
        goto cleanup_buff2;
    }

    // Create the file read content
    file_content_t * p_content = (file_content_t *)malloc(sizeof(file_content_t));
    if (UV_INVALID_ALLOC == verify_alloc(p_content))
    {
        *p_code = OP_FAILURE;
        goto cleanup_hash;
    }

    // Duplicate the file path, the verified path is not needed inside the
    // contents structure
    char * p_file_path = strdup(p_path->p_path);
    if (UV_INVALID_ALLOC == verify_alloc(p_file_path))
    {
        *p_code = OP_FAILURE;
        goto cleanup_content;
    }

    // Save data into the content structure to return
    *p_content = (file_content_t){
        .p_stream       = p_buff,
        .p_hash         = p_hash,
        .stream_size    = str_len,
        .p_path         = p_file_path
    };

    *p_code = OP_SUCCESS;
    free(p_buffer);
    return p_content;

cleanup_content:
    f_destroy_content(&p_content);
cleanup_hash:
    hash_destroy(&p_hash);
cleanup_buff2:
    free(p_buff);
    p_buff = NULL;
cleanup_buff:
    free(p_buffer);
    p_buffer = NULL;
ret_null:
    return NULL;
}


/*!
 * @brief Destroy the file_content_t object
 * @param pp_content Double pointer to the file_content_t object
 */
void f_destroy_content(file_content_t ** pp_content)
{
    if (NULL == pp_content)
    {
        return;
    }

    file_content_t * p_content = * pp_content;
    if (NULL == p_content)
    {
        return;
    }

    hash_destroy(& p_content->p_hash);
    free(p_content->p_stream);
    free(p_content->p_path);
    * p_content = (file_content_t){
        .p_stream    = NULL,
        .p_path      = NULL,
        .p_hash      = NULL,
        .stream_size = 0
    };
    free(p_content);
    * pp_content = NULL;
}

/*!
 * @brief Attempt to join and resolve the two paths provided. The function will
 * handle the "/" regardless if both or neither paths to join have the "/".
 *
 * @param p_root String of p_root path
 * @param root_length Length of p_root path
 * @param p_child String of p_child path
 * @param child_length Length of p_child path
 * @return Pointer to the resolved path or NULL if failure
 */
DEBUG_STATIC char * join_and_resolve_paths(const char * p_root,
                                           size_t root_length,
                                           const char * p_child,
                                           size_t child_length)
{
    if ((NULL == p_root) || (NULL == p_child))
    {
        goto ret_null;
    }

    char * new_path = join_paths(p_root, root_length, p_child, child_length);
    if (NULL == new_path)
    {
        goto ret_null;
    }

    char * p_abs_path = realpath(new_path, NULL);
    if (NULL == p_abs_path)
    {
        debug_print("[!] %s did not resolve\n", new_path);
        goto cleanup; // Path did not resolve
    }

    free(new_path);
    return p_abs_path;

cleanup:
    free(new_path);
ret_null:
    return NULL;
}

/*!
 * @brief Function handles concatenating two paths together while taking into
 * account the size of the new string to ensure that it does not exceed the
 * max path string limit
 *
 * @param p_root String of p_root path
 * @param root_length Length of p_root path
 * @param p_child String of p_child path
 * @param child_length Length of p_child path
 * @return Pointer to the new joined path or NULL if failure
 */
static char * join_paths(const char * p_root,
                         size_t root_length,
                         const char * p_child,
                         size_t child_length)
{
    if ((NULL == p_root) || (NULL == p_child))
    {
        goto ret_null;
    }

    bool b_omit_root = false;
    bool b_omit_child = false;

    if ('/' == p_root[root_length])
    {
        b_omit_root = true;
    }
    if ('/' == p_child[0])
    {
        b_omit_child = true;
    }

    // concat the strings while omitting the "/" of both strings if present
    char new_path[PATH_MAX] = {0};
    strncat(new_path, p_root, (b_omit_root) ? root_length - 1 : root_length);
    if (!b_omit_child)
    {
        strncat(new_path, "/", 2);
    }
    strncat(new_path, p_child, child_length);

    char * joined_path = strdup(new_path);
    if (UV_INVALID_ALLOC == verify_alloc(joined_path))
    {
        goto ret_null;
    }
    return joined_path;

ret_null:
    return NULL;
}

/*!
 * @brief Reallocate the buffer and memset the newly added portion
 * of the buffer
 *
 * @param p_buffer Buffer to resize
 * @param p_size Size to resize by
 * @param offset Offset is the last portion initialized previously
 * @return New pointer to the new buffer or NULL
 */
static uint8_t * realloc_buff(uint8_t * p_buffer, size_t * p_size, size_t offset)
{
    *p_size = *p_size * 2;
    uint8_t * p_buff = (uint8_t *)realloc(p_buffer, *p_size);
    if (UV_INVALID_ALLOC == verify_alloc(p_buff))
    {
        free(p_buffer);
        p_buffer = NULL;
        return NULL;
    }

    // Initialized the new portion of the realloc
    memset(p_buff + offset, 0, (*p_size - offset));
    return p_buff;
}

/*!
 * @brief Get the size of the file and the number of digits that represents
 * the file size
 *
 * @param p_path Pointer to the verified file path
 * @param name Name of the file
 * @param num_len Number of integers that represents the file size
 * @return Size of the file
 */
static size_t get_file_size(verified_path_t * p_path,
                            char name[256],
                            uint16_t * num_len)
{
    struct stat stat_buff = {0};
    char path[PATH_MAX] = {0};

    // Both p_path and name are guaranteed to be null terminated
    strcat(path, p_path->p_path);
    strcat(path, "/");
    strcat(path, name);

    if (-1 == stat(path, &stat_buff))
    {
        debug_print_err("[!] Unable to get stats for %s\n:Error: %s\n",
                        p_path->p_path, strerror(errno));
        return 0;
    }

    *num_len = 1;
    size_t size_copy = (size_t)stat_buff.st_size;
    while (*num_len > 9)
    {
        size_copy /= 10;
        (*num_len)++;
    }

    return (size_t)stat_buff.st_size;
}
