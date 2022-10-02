#ifndef BSLE_GALINDEZ_INCLUDE_SERVER_CTRL_H_
#define BSLE_GALINDEZ_INCLUDE_SERVER_CTRL_H_
#ifdef __cplusplus
extern "C" {
#endif //END __cplusplus
// HEADER GUARD
#include <stdio.h>
#include <stdint.h>

#include <server_db.h>


typedef enum
{
    NO_PAYLOAD,
    STD_PAYLOAD,
    USER_PAYLOAD
} payload_type_t;

typedef struct
{
    uint16_t       path_len;
    char *         p_path;

    // Byte stream are only populated for PutRemote command
    uint8_t *       p_byte_stream;
    uint8_t *       p_hash_stream;

    uint64_t         byte_stream_len;
} std_payload_t;

typedef struct
{
    usr_act_t   user_flag;
    perms_t     user_perm;  // Only set for user creation
    uint16_t    username_len;
    char *      p_username;

    // Password is only set for user creation
    uint16_t    passwd_len;
    char *      p_passwd;
} user_payload_t;

typedef struct
{
    act_t           opt_code;       // 1 byte
    usr_act_t       user_flag;      // 1 byte
    uint16_t        _reserved;
    uint16_t        username_len;
    uint16_t        passwd_len;
    uint32_t        session_id;
    char *          p_username;
    char *          p_passwd;
    uint64_t        payload_len;  // Size of everything but wire header

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

act_resp_t * ctrl_populate_resp(ret_codes_t code);

// HEADER GUARD
#ifdef __cplusplus
}
#endif // END __cplusplus
#endif //BSLE_GALINDEZ_INCLUDE_SERVER_CTRL_H_
