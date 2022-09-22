#ifndef BSLE_GALINDEZ_INCLUDE_SERVER_CRYPTO_H_
#define BSLE_GALINDEZ_INCLUDE_SERVER_CRYPTO_H_
#ifdef __cplusplus
extern "C" {
#endif //END __cplusplus
// HEADER GUARD
#include <openssl/sha.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include <server_file_api.h>
#include <utils.h>

typedef struct
{
    size_t      size;
    uint8_t *   array;
} hash_t;

hash_t * hash_pass(const unsigned char * p_input, size_t length);
void hash_init_db(char * p_dir_path, size_t dir_length);
bool hash_pass_match(hash_t * p_hash, const char * p_input, size_t length);
void hash_destroy(hash_t ** pp_hash);


// HEADER GUARD
#ifdef __cplusplus
}
#endif // END __cplusplus
#endif //BSLE_GALINDEZ_INCLUDE_SERVER_CRYPTO_H_
