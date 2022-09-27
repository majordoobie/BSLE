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
} server_defaults_t;

//static const char * OP_1 = "Server action was successful";
//static const char * OP_2 = "Provided Session ID was invalid or expired";
//static const char * OP_3 = "User associated with provided Session ID has insufficient permissions to perform the action";
//static const char * OP_4 = "User could not be created because it already exists";
//static const char * OP_5 = "File could not be created because it already exists";
//static const char * OP_6 = "Username must be between 3 and 20 characters and password must be between 6 and 32 characters";
//static const char * OP_255 = "Server action failed";

typedef enum
{
    OP_SUCCESS             = 1,
    OP_SESSION_ERROR       = 2,
    OP_PERMISSION_ERROR    = 3,
    OP_USER_EXISTS         = 4,
    OP_FILE_EXISTS         = 5,
    OP_CRED_RULE_ERROR     = 6,
    OP_FAILURE             = 255
} server_error_codes_t;

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
