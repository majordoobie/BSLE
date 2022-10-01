#ifndef BSLE_GALINDEZ_INCLUDE_SERVER_H_
#define BSLE_GALINDEZ_INCLUDE_SERVER_H_
#include <server_crypto.h>

// Values had to be moved into macros to provide the ability for
// dynamic sscanf parsing otherwise these belong in server_defaults_t
#define MAX_USERNAME_LEN 20
#define SHA256_DIGEST_LEN 64

typedef enum
{
    MIN_PORT            = 1024,       // Ports 1024+ are user defined ports
    MAX_PORT            = 0xFFFF,
    BACK_LOG            = 1024,
    MIN_USERNAME_LEN    = 3,
    MIN_PASSWD_LEN      = 6,
    MAX_PASSWD_LEN      = 32,
    MAX_WIRE_HEADER     = 64,
    MAX_WIRE_PAYLOAD    = 2048,
    MAX_TRX_PAYLOLAD    = 1016,
} server_defaults_t;

typedef enum
{
    H_OPCODE        = 1,
    H_USER_FLAG     = 1,
    H_RESERVED      = 2,
    H_USERNAME_LEN  = 2,
    H_PASSWORD_LEN  = 2,
    H_SESSION_ID    = 4,
} wire_header_t;

typedef enum
{
    OP_SUCCESS             = 1,
    OP_SESSION_ERROR       = 2,
    OP_PERMISSION_ERROR    = 3,
    OP_USER_EXISTS         = 4,
    OP_FILE_EXISTS         = 5,
    OP_CRED_RULE_ERROR     = 6,
    OP_USER_AUTH           = 7,
    OP_DIR_NOT_EMPTY       = 8,
    OP_RESOLVE_ERROR       = 9,
    OP_PATH_NOT_DIR        = 10,
    OP_PATH_NOT_FILE       = 11,
    OP_DIR_EXISTS          = 12,
    OP_IO_ERROR            = 254,
    OP_FAILURE             = 255
} ret_codes_t;

typedef enum
{
    ACT_USER_OPERATION          = 1,
    ACT_DELETE_REMOTE_FILE      = 2,
    ACT_LIST_REMOTE_DIRECTORY   = 3,
    ACT_GET_REMOTE_FILE         = 4,
    ACT_MAKE_REMOTE_DIRECTORY   = 5,
    ACT_PUT_REMOTE_FILE         = 6,
} act_t;

typedef enum
{
    USR_ACT_CREATE_USER         = 1,
    USR_ACT_DELETE_USER         = 2
} usr_act_t;

typedef enum
{
    DEFAULT_PORT    = 31337,
    DEFAULT_TIMEOUT = 10,
} args_default_t;

typedef enum
{
    READ        = 1,
    READ_WRITE  = 2,
    ADMIN       = 3
} perms_t;

typedef struct
{
    char *   p_username;
    perms_t  permission;
    hash_t * p_hash;
} user_account_t;

#endif //BSLE_GALINDEZ_INCLUDE_SERVER_H_
