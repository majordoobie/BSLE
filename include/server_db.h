#ifndef BSLE_GALINDEZ_SRC_SERVER_SERVER_INIT_H_
#define BSLE_GALINDEZ_SRC_SERVER_SERVER_INIT_H_
#ifdef __cplusplus
extern "C" {
#endif //END __cplusplus
// HEADER GUARD

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <utils.h>
#include <server.h>
#include <server_file_api.h>
#include <server_crypto.h>
#include <hashtable.h>

typedef struct
{
    htable_t *          users_htable;
    verified_path_t *   p_home_dir;
} db_t;

db_t * db_init(verified_path_t * p_home_dir);

void db_shutdown(db_t ** pp_db);

/*!
 * @brief The function looks up the user to see if they exist then the
 * password provided for authentication is hashed and checked against the
 * stored hash. If they match the user account is returned.
 *
 * @param p_db Server database object
 * @param p_user Pointer to save the authenticated user to
 * @param username Username provided for authentication
 * @param passwd Password provided for authentication
 * @retval OP_SUCCESS On successful authentication
 * @retval OP_USER_AUTH On user lookup failure or authentication failure
 * @retval OP_FAILURE Memory or API failures
 */
server_error_codes_t db_authenticate_user(db_t * p_db,
                                          user_account_t * p_user,
                                          const char * username,
                                          const char * passwd);

/*!
 * @brief Function attempts to create a new user and add it to the user
 * database. If the username or password do not meet the size criteria a
 * credential error is returned.
 *
 * @param p_db Pointer to the database object
 * @param username Pointer to the username
 * @param passwd Pointer to the password
 * @param permission Permission of the new user
 * @return OP_SUCCESS upon successful creation. OP_USER_EXISTS if user already
 * exists. OP_CRED_RULE_ERROR if username or password trigger a length rule.
 * Otherwise a OP_FAILURE is returned if some internal error occurred.
 */
server_error_codes_t db_create_user(db_t * p_db,
                                    const char * username,
                                    const char * passwd,
                                    perms_t permission);

// HEADER GUARD
#ifdef __cplusplus
}
#endif // END __cplusplus
#endif //BSLE_GALINDEZ_SRC_SERVER_SERVER_INIT_H_
