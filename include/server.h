#ifndef BSLE_GALINDEZ_SRC_SERVER_SERVER_H_
#define BSLE_GALINDEZ_SRC_SERVER_SERVER_H_

#ifdef __cplusplus
extern "C" {
#endif //END __cplusplus
#include <stdint.h>
#include <utils.h>
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

typedef struct args_t
{
    uint32_t port;
    uint32_t timeout;
    char * home_directory;
} args_t;

typedef struct byte_array_t
{
    size_t size;
    uint8_t * array;
} byte_array_t;

void free_args(args_t ** args_p);
args_t * parse_args(int argc, char ** argv);


byte_array_t * hash_pass(char *input, size_t length);
void free_b_array(byte_array_t ** array);


#ifdef __cplusplus
}
#endif // END __cplusplus


#endif //BSLE_GALINDEZ_SRC_SERVER_SERVER_H_
