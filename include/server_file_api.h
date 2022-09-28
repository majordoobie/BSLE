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
#include <stdint.h>
#include <stdio.h>
#include <errno.h>

#include <utils.h>
#include <server_crypto.h>
#include "server.h"

typedef struct verified_path verified_path_t;

// Structure is used when reading contents. It holds the file byte stream
// along with its hash, its path and the streams size.
typedef struct
{
    hash_t *    p_hash;
    uint8_t *   p_stream;
    size_t      stream_size;
    char *      p_path;
} file_content_t;

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
void f_path_repr(verified_path_t * p_path, char * path_repr, size_t path_size);

/*!
 * @brief Function creates a verified path representing the home dir. A
 * verified path is a object that contains the path to path that exists
 * and exists with in the home dir.
 *
 * @param p_home_dir Pointer to the home dir string
 * @param dir_size Size of the home dir string
 * @return Verified path if path exists else NULL
 */
verified_path_t * f_set_home_dir(const char * p_home_dir, size_t dir_size);

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
 * @brief Exact same function as f_path_resolve except that a verified path
 * of the homedir is used.
 *
 * @param p_home_dir verified_path_t object of the home dir
 * @param p_child Pointer to the child path to resolve
 * @return Verified path if the path is valid or else NULL
 */
verified_path_t * f_ver_path_resolve(verified_path_t * p_home_dir, const char * p_child);

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
verified_path_t * f_valid_resolve(const char * p_home_dir, const char * p_child);

/*!
 * @brief Exact same function as f_valid_resolve except that a verified path
 * of the homedir is used.
 *
 * @param p_home_dir verified_path_t object of the home dir
 * @param p_child Pointer to the child path to resolve
 * @return Verified path if the path is valid or else NULL
 */
verified_path_t * f_ver_valid_resolve(verified_path_t * p_home_dir, const char * p_child);


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
 * @brief Destroy the file_content_t object
 * @param pp_content Double pointer to the file_content_t object
 */
void f_destroy_content(file_content_t ** pp_content);


/*
 *
 *
 * User operations
 *
 *
 */

/*!
 * @brief Read wrapper is used to read the verified file path. If successful,
 * the data read is hashed and all the metadata about the stream is added
 * into the file_content_t object.
 *
 * @param p_path Pointer to a verified_path_t object
 * @return file_content_t object if successful, otherwise NULL
 */
file_content_t * f_read_file(verified_path_t * p_path);


/*!
 * @brief Simple wrapper to write the data stream to the verified file path
 *
 * @param p_path Pointer to a verified_file_t object
 * @param p_stream Pointer to a byte stream
 * @param stream_size Number of bytes in the byte stream
 * @return FILE_OP_SUCCESS if operation succeeded, otherwise FILE_OP_FAILURE
 */
server_error_codes_t f_write_file(verified_path_t * p_path, uint8_t * p_stream, size_t stream_size);

/*!
 * @brief Simple wrapper for creating a directory using the verified_path_t
 * object
 *
 * @param p_path Pointer to a verified_path_t object
 * @return file_op_t indicating if the operation failed or succeeded
 */
server_error_codes_t f_create_dir(verified_path_t * p_path);

server_error_codes_t f_del_file(verified_path_t * p_path);

// HEADER GUARD
#ifdef __cplusplus
}
#endif // END __cplusplus
#endif //BSLE_GALINDEZ_INCLUDE_SERVER_FILE_API_H_
