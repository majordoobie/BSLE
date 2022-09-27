#ifndef BSLE_GALINDEZ_INCLUDE_SERVER_H_
#define BSLE_GALINDEZ_INCLUDE_SERVER_H_
typedef enum
{
    MIN_PORT            = 1024,       // Ports 1024+ are user defined ports
    MAX_PORT            = 0xFFFF,
    BACK_LOG            = 1024,
    MIN_USERNAME_LEN    = 3,
} server_defaults_t;
#define MAX_USERNAME_LEN 20
#define PERM_LEN 1
#define SHA256_DIGEST_LEN 64

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
    char *      username;
    perms_t *   permission;
    uint8_t *   pw_hash;
    size_t      hash_size;
} user_account_t;

#endif //BSLE_GALINDEZ_INCLUDE_SERVER_H_
