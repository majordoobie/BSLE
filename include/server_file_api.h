#ifndef BSLE_GALINDEZ_INCLUDE_SERVER_FILE_API_H_
#define BSLE_GALINDEZ_INCLUDE_SERVER_FILE_API_H_
#ifdef __cplusplus
extern "C" {
#endif //END __cplusplus
// HEADER GUARD
#include <stdlib.h>
#include <string.h>
#include <linux/limits.h>
#include <stdbool.h>
#include <libgen.h>
#include <sys/stat.h>

#include <utils.h>

typedef enum
{
    FILE_OP_SUCCESS,
    FILE_OP_FAILURE
} file_op_t;

typedef struct verified_path verified_path_t;

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
 * character limit is exceeded, if the file does not exist or if the file
 * exists but outside the home directory.
 */
verified_path_t * f_path_resolve(const char * p_home_dir, const char * p_child);

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
 * character limit is exceeded, if the file does not exist or if the file exists
 * but outside the home directory.
 */
verified_path_t * f_dir_resolve(const char * p_home_dir, const char * p_child);

/*!
 * @brief Destroy the verified_path_t object
 *
 * @param pp_path Double pointer to the verified_path_t object
 */
void f_destroy_path(verified_path_t ** pp_path);

/*!
 * @brief Simple wrapper for opening the verified_path_t object
 *
 * @param p_path Pointer to the verified_path_t object
 * @param p_read_mode Mode to open the verified_path_t object with
 * @return File object or NULL if the path cannot be opened
 */
FILE * f_open_file(verified_path_t * p_path, const char * p_read_mode);

/*!
 * @brief Simple wrapper for creating a directory using the verified_path_t
 * object
 *
 * @param p_path Pointer to a verified_path_t object
 * @return file_op_t indicating if the operation failed or succeeded
 */
file_op_t f_create_dir(verified_path_t * p_path);

// HEADER GUARD
#ifdef __cplusplus
}
#endif // END __cplusplus
#endif //BSLE_GALINDEZ_INCLUDE_SERVER_FILE_API_H_
