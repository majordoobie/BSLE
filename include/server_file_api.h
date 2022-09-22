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

#include <utils.h>

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
 * character limit is exceeded, if the file does not exist or if the file exists but outside the home directory.
 */
verified_path_t * f_path_resolve(const char * p_home_dir, const char * p_child);

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


// HEADER GUARD
#ifdef __cplusplus
}
#endif // END __cplusplus
#endif //BSLE_GALINDEZ_INCLUDE_SERVER_FILE_API_H_
