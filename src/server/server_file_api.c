#include <server_file_api.h>

struct verified_path
{
    char * p_path;
};

DEBUG_STATIC char * join_paths(const char * p_root, size_t root_length, const char * p_child, size_t child_length);

// Bytes needed to account for the "/" and a "\0"
#define SLASH_PLUS_NULL 2

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

    char * p_join_path = join_paths(p_home_dir, home_dir_len, p_child, child_len);
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
