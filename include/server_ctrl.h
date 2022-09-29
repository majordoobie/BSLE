#ifndef BSLE_GALINDEZ_INCLUDE_SERVER_CTRL_H_
#define BSLE_GALINDEZ_INCLUDE_SERVER_CTRL_H_
#ifdef __cplusplus
extern "C" {
#endif //END __cplusplus
// HEADER GUARD
#include <stdio.h>
#include <stdint.h>

#include <server_db.h>

typedef struct
{
    const char *    msg;
    ret_codes_t     result;
} act_resp_t;

typedef enum
{
    STD_PAYLOAD,
    USER_PAYLOAD
} payload_type_t;

typedef struct
{
    size_t          path_len;
    char *          p_path;

    // Byte stream are only populated for PutRemote command
    uint8_t *       p_byte_stream;
    size_t          byte_stream_len;
} std_payload_t;

typedef struct
{
    usr_act_t   user_flag;
    perms_t     user_perm;  // Only set for user creation
    size_t      username_len;
    char *      p_username;

    // Password is only set for user creation
    size_t      passwd_len;
    char *      p_passwd;
} user_payload_t;

typedef struct
{
    act_t           opt_code;
    size_t          username_len;
    size_t          passwd_len;
    uint16_t        session_id;
    char *          p_username;
    char *          p_passwd;
    size_t          payload_len;  // Size of everything but wire header

    payload_type_t type;
    union
    {
        std_payload_t *  p_std_payload;
        user_payload_t * p_user_payload;
    };
} wire_payload_t;

/*!
 * @brief Destroy the payload and response objects
 *
 * @param pp_payload Double pointer to the payload object
 * @param pp_res Double pointer to the response object
 */
void ctrl_destroy(wire_payload_t ** pp_payload, act_resp_t ** pp_res);

act_resp_t * ctrl_parse_action(db_t * p_user_db, wire_payload_t * p_ld);

// HEADER GUARD
#ifdef __cplusplus
}
#endif // END __cplusplus
#endif //BSLE_GALINDEZ_INCLUDE_SERVER_CTRL_H_
