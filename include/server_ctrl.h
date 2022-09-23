#ifndef BSLE_GALINDEZ_INCLUDE_SERVER_CTRL_H_
#define BSLE_GALINDEZ_INCLUDE_SERVER_CTRL_H_
#ifdef __cplusplus
extern "C" {
#endif //END __cplusplus
// HEADER GUARD

#include <stdint.h>
#include <stdbool.h>

#include <utils.h>
#include <server_file_api.h>
#include <server_crypto.h>
#include <server_init.h>

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

// HEADER GUARD
#ifdef __cplusplus
}
#endif // END __cplusplus
#endif //BSLE_GALINDEZ_INCLUDE_SERVER_CTRL_H_
