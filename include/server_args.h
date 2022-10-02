#ifndef BSLE_GALINDEZ_INCLUDE_SERVER_ARGS_H_
#define BSLE_GALINDEZ_INCLUDE_SERVER_ARGS_H_
#ifdef __cplusplus
extern "C" {
#endif //END __cplusplus
// HEADER GUARD
#include <ctype.h>
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <utils.h>
#include <server.h>
#include <server_file_api.h>

typedef struct
{
    uint32_t            port;
    uint8_t             timeout;
    verified_path_t *   p_home_directory;
} args_t;

void args_destroy(args_t ** pp_args);
args_t * args_parse(int argc, char ** argv);

// HEADER GUARD
#ifdef __cplusplus
}
#endif // END __cplusplus
#endif //BSLE_GALINDEZ_INCLUDE_SERVER_ARGS_H_
