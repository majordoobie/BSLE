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

#include <server_ctrl.h>
#include <utils.h>

typedef struct
{
    uint32_t    port;
    uint32_t    timeout;
    char *      p_home_directory;
} args_t;

void free_args(args_t ** pp_args);
args_t * parse_args(int argc, char ** argv);

// HEADER GUARD
#ifdef __cplusplus
}
#endif // END __cplusplus
#endif //BSLE_GALINDEZ_INCLUDE_SERVER_ARGS_H_
