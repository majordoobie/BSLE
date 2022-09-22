#include <server_file_api.h>

DEBUG_STATIC char * join_paths(const char * p_root, size_t root_length, const char * p_child, size_t child_length);
DEBUG_STATIC FILE * get_file(const char * p_home_dir, size_t home_length, const char * p_file, const char * p_read_mode);

FILE * s_open_file(const char * p_home_dir,
                   const char * p_file_path,
                   const char * p_read_mode)
{
    size_t home_dir_len = strlen(p_home_dir);
    size_t file_path_len = strlen(p_file_path);
    if ((home_dir_len + file_path_len + 2) > PATH_MAX)
    {
        goto ret_null;
    }

    char * p_join_path = join_paths(p_home_dir, home_dir_len, p_file_path, file_path_len);

    if (NULL == p_join_path)
    {
        goto ret_null;
    }

    FILE * h_file = get_file(p_home_dir, home_dir_len, p_file_path, p_read_mode);
    if (NULL == h_file)
    {
        goto cleanup;
    }
    return h_file;

cleanup:
    free(p_join_path);
ret_null:
    return NULL;
}

/*!
 * @brief Verify if the p_file path provided resolves inside the home directory
 * path. If it does not, return NULL. Otherwise, attempt to open the p_file
 * provided and return a FILE object.
 *
 * @param p_home_dir Pointer to the home directory of the server
 * @param home_length Length of the home directory path
 * @param p_file Pointer to the p_file path to open
 * @param p_read_mode Mode to open the p_file path
 * @return FILE object if successful; otherwise NULL
 */
DEBUG_STATIC FILE * get_file(const char * p_home_dir, size_t home_length, const char * p_file, const char * p_read_mode)
{
    if ((NULL == p_home_dir) || (NULL == p_file))
    {
        goto ret_null;
    }

    char * p_abs_path = realpath(p_file, NULL);
    if (UV_INVALID_ALLOC == verify_alloc(p_abs_path))
    {
        goto ret_null;
    }

    if (0 != strncmp(p_home_dir, p_abs_path, home_length))
    {
        fprintf(stderr, "[!] File path provided does not exist "
                        "within the home directory of the server\n->"
                        "[DIR] %s\n->[FILE]%s\n", p_home_dir, p_abs_path);
        goto cleanup;
    }

    FILE * h_file_obj = fopen(p_abs_path, p_read_mode);
    if (NULL == h_file_obj)
    {
        perror("fopen");
        goto cleanup;
    }

    free(p_abs_path);
    return h_file_obj;


cleanup:
    free(p_abs_path);
ret_null:
    return NULL;
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
DEBUG_STATIC char * join_paths(const char * p_root, size_t root_length, const char * p_child, size_t child_length)
{
    if ((NULL == p_root) || (NULL == p_child))
    {
        goto ret_null;
    }

    if ((root_length + child_length + 2) > PATH_MAX)
    {
        fprintf(stderr, "[!] Resolve path exceeds the file path "
                        "character limit\n");
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

    char * p_abs_path = realpath(new_path, NULL);
    if (UV_INVALID_ALLOC == verify_alloc(p_abs_path))
    {
        perror("\nrealpath");
        goto cleanup;
    }

    return p_abs_path;

cleanup:
    free(p_abs_path);
ret_null:
    return NULL;
}
