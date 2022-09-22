#include <stdlib.h>
#include <string.h>

#include <server.h>
#include <linux/limits.h>

FILE * s_get_file(const char * home_dir, size_t home_length, const char * file, const char * read_mode)
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
                        "within the home directory of the server\n");
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

char * s_join_paths(char * root, size_t root_length, char * child, size_t child_length)
{
    if ((NULL == root) || (NULL == child) || ((root_length + child_length + 1) > PATH_MAX))
    {
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
