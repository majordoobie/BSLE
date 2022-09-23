#include <server_file_api.h>

struct verified_path
{
    char * p_path;
};

DEBUG_STATIC char * join_and_resolve_paths(const char * p_root, size_t root_length, const char * p_child, size_t child_length);
static char * join_paths(const char * p_root, size_t root_length, const char * p_child, size_t child_length);

// Bytes needed to account for the "/" and a "\0"
#define SLASH_PLUS_NULL 2

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
    char * child_dir =  dirname(p_child_cpy);
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
    verified_path_t * p_path = (verified_path_t *)malloc(sizeof(verified_path_t));
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

    char * p_join_path = join_and_resolve_paths(p_home_dir, home_dir_len, p_child, child_len);
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

    verified_path_t * p_path = (verified_path_t *)malloc(sizeof(verified_path_t));
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
    verified_path_t * p_path = *pp_path;
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
    *pp_path = NULL;
}

/*!
 * @brief Simple wrapper for creating a directory using the verified_path_t
 * object
 *
 * @param p_path Pointer to a verified_path_t object
 * @return file_op_t indicating if the operation failed or succeeded
 */
file_op_t f_create_dir(verified_path_t * p_path)
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
    return FILE_OP_SUCCESS;

ret_null:
    return FILE_OP_FAILURE;
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
file_op_t f_write_file(verified_path_t * p_path, uint8_t * p_stream, size_t stream_size)
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
    return FILE_OP_SUCCESS;

cleanup:
    fclose(h_path);
ret_null:
    return FILE_OP_FAILURE;
}

/*!
 * @brief Read wrapper is used to read the verified file path. If successful,
 * the data read is hashed and all the metadata about the stream is added
 * into the file_content_t object.
 *
 * @param p_path Pointer to a verified_path_t object
 * @return file_content_t object if successful, otherwise NULL
 */
file_content_t * f_read_file(verified_path_t * p_path)
{
    if (NULL == p_path)
    {
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
    uint8_t * p_byte_array = (uint8_t *)calloc((unsigned long)file_size, sizeof(uint8_t));
    if (UV_INVALID_ALLOC == verify_alloc(p_byte_array))
    {
        goto cleanup_close;
    }

    // Read the contents into the p_byte_array created
    size_t bytes_read = fread(p_byte_array, sizeof(uint8_t),(unsigned long)file_size, h_path);
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
        goto cleanup_array;
    }

    // Create the file read content
    file_content_t * p_content = (file_content_t *)malloc(sizeof(file_content_t));
    if (UV_INVALID_ALLOC == verify_alloc(p_content))
    {
        goto cleanup_hash;
    }

    // Duplicate the file path, the verified path is not needed inside the
    // contents structure
    char * p_file_path = strdup(p_path->p_path);
    if (UV_INVALID_ALLOC == verify_alloc(p_file_path))
    {
        goto cleanup_content;
    }

    // Save data into the content structure to return
    *p_content = (file_content_t){
        .p_stream       = p_byte_array,
        .p_hash         = p_hash,
        .stream_size    = bytes_read,
        .p_path         = p_file_path
    };

    return p_content;

cleanup_content:
    free(p_content); // Content is not populated here so no destroy is called
cleanup_hash:
    hash_destroy(&p_hash);
cleanup_array:
    free(p_byte_array);
cleanup_close:
    fclose(h_path);
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

    file_content_t * p_content = *pp_content;
    if (NULL == p_content)
    {
        return;
    }

    hash_destroy(&p_content->p_hash);
    free(p_content->p_stream);
    free(p_content->p_path);
    *p_content = (file_content_t){
        .p_stream    = NULL,
        .p_path      = NULL,
        .p_hash      = NULL,
        .stream_size = 0
    };
    free(p_content);
    *pp_content = NULL;
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
DEBUG_STATIC char * join_and_resolve_paths(const char * p_root, size_t root_length, const char * p_child, size_t child_length)
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
static char * join_paths(const char * p_root, size_t root_length, const char * p_child, size_t child_length)
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
