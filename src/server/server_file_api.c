#include <stdlib.h>
#include <string.h>
#include <linux/limits.h>

#include <server.h>

DEBUG_STATIC char * join_paths(const char * root, size_t root_length, const char * child, size_t child_length);
DEBUG_STATIC FILE * get_file(const char * home_dir, size_t home_length, const char * file, const char * read_mode);

FILE * s_open_file(const char * home_dir,
                   const char * file_path,
                   const char * read_mode)
{
    size_t home_dir_len = strlen(home_dir);
    size_t file_path_len = strlen(file_path);
    if ((home_dir_len + file_path_len + 2) > PATH_MAX)
    {
        goto ret_null;
    }

    char * join_path = join_paths(home_dir, home_dir_len, file_path, file_path_len);
    if (NULL == join_path)
    {
        goto ret_null;
    }

    FILE * file = get_file(home_dir, home_dir_len, file_path, read_mode);
    if (NULL == file)
    {
        goto cleanup;
    }
    return file;

cleanup:
    free(join_path);
ret_null:
    return NULL;
}

/*!
 * @brief Verify if the file path provided resolves inside the home directory
 * path. If it does not, return NULL. Otherwise, attempt to open the file
 * provided and return a FILE object.
 *
 * @param home_dir Pointer to the home directory of the server
 * @param home_length Length of the home directory path
 * @param file Pointer to the file path to open
 * @param read_mode Mode to open the file path
 * @return FILE object if successful; otherwise NULL
 */
DEBUG_STATIC FILE * get_file(const char * home_dir, size_t home_length, const char * file, const char * read_mode)
{
    if ((NULL == home_dir) || (NULL == file))
    {
        goto ret_null;
    }

    char * abs_path = realpath(file, NULL);
    if (UV_INVALID_ALLOC == verify_alloc(abs_path))
    {
        goto ret_null;
    }

    if (0 != strncmp(home_dir, abs_path, home_length))
    {
        fprintf(stderr, "[!] File path provided does not exist "
                        "within the home directory of the server\n->"
                        "[DIR] %s\n->[FILE]%s\n", home_dir, abs_path);
        goto cleanup;
    }

    FILE * file_obj = fopen(abs_path, read_mode);
    if (NULL == file_obj)
    {
        perror("fopen");
        goto cleanup;
    }

    free(abs_path);
    return file_obj;


cleanup:
    free(abs_path);
ret_null:
    return NULL;
}

/*!
 * @brief Attempt to join and resolve the two paths provided. The function will
 * handle the "/" regardless if both or neither paths to join have the "/".
 *
 * @param root String of root path
 * @param root_length Length of root path
 * @param child String of child path
 * @param child_length Length of child path
 * @return Pointer to the resolved path or NULL if failure
 */
DEBUG_STATIC char * join_paths(const char * root, size_t root_length, const char * child, size_t child_length)
{
    if ((NULL == root) || (NULL == child))
    {
        goto ret_null;
    }

    if ((root_length + child_length + 2) > PATH_MAX)
    {
        fprintf(stderr, "[!] Resolve path exceeds the file path "
                        "character limit\n");
        goto ret_null;
    }

    bool omit_root = false;
    bool omit_child = false;

    if ('/' == root[root_length])
    {
        omit_root = true;
    }
    if ('/' == child[0])
    {
        omit_child = true;
    }

    // concat the strings while omitting the "/" of both strings if present
    char new_path[PATH_MAX] = {0};
    strncat(new_path, root, (omit_root) ? root_length - 1 : root_length);
    if (!omit_child)
    {
        strncat(new_path, "/", 2);
    }
    strncat(new_path, child, child_length);

    char * abs_path = realpath(new_path, NULL);
    if (UV_INVALID_ALLOC == verify_alloc(abs_path))
    {
        perror("\nrealpath");
        goto cleanup;
    }

    return abs_path;

cleanup:
    free(abs_path);
ret_null:
    return NULL;
}
