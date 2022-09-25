#ifndef BSLE_GALINDEZ_SRC_SERVER_SERVER_INIT_H_
#define BSLE_GALINDEZ_SRC_SERVER_SERVER_INIT_H_
#ifdef __cplusplus
extern "C" {
#endif //END __cplusplus
// HEADER GUARD

#include <stdint.h>
#include <stdbool.h>

#include <utils.h>
#include <server.h>
#include <server_file_api.h>
#include <server_crypto.h>

int8_t hash_init_db(char * p_home_dir, size_t dir_length);


// HEADER GUARD
#ifdef __cplusplus
}
#endif // END __cplusplus
#endif //BSLE_GALINDEZ_SRC_SERVER_SERVER_INIT_H_
