#ifndef BSLE_GALINDEZ_INCLUDE_SERVER_H_
#define BSLE_GALINDEZ_INCLUDE_SERVER_H_
typedef enum
{
    MIN_PORT    = 1024,       // Ports 1024+ are user defined ports
    MAX_PORT    = 0xFFFF,
    BACK_LOG    = 1024
} server_defaults_t;

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
#endif //BSLE_GALINDEZ_INCLUDE_SERVER_H_
