#ifndef BSLE_GALINDEZ_SRC_SERVER_SERVER_H_
#define BSLE_GALINDEZ_SRC_SERVER_SERVER_H_

#ifdef __cplusplus
extern "C" {
#endif //END __cplusplus
#include <stdint.h>
#include <stdbool.h>

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

typedef struct
{
    uint32_t    port;
    uint32_t    timeout;
    char *      home_directory;
} args_t;

typedef struct
{
    size_t      size;
    uint8_t *   array;
} hash_t;

void free_args(args_t ** args_p);
args_t * parse_args(int argc, char ** argv);

char * s_join_paths(char * root, size_t root_length, char * child, size_t child_length);
FILE * s_get_file(const char * home_dir, size_t home_length, const char * file, const char * read_mode);

hash_t * hash_pass(const unsigned char *input, size_t length);
void hash_init_db(char *directory, size_t dir_length);
bool hash_pass_match(hash_t * hash, const char *input, size_t length);
void hash_destroy(hash_t ** hash);


#ifdef __cplusplus
}
#endif // END __cplusplus


#endif //BSLE_GALINDEZ_SRC_SERVER_SERVER_H_
