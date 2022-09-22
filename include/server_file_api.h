#ifndef BSLE_GALINDEZ_INCLUDE_SERVER_FILE_API_H_
#define BSLE_GALINDEZ_INCLUDE_SERVER_FILE_API_H_
#ifdef __cplusplus
extern "C" {
#endif //END __cplusplus
// HEADER GUARD
#include <stdlib.h>
#include <string.h>
#include <linux/limits.h>
#include <stdbool.h>

#include <utils.h>

FILE * s_open_file(const char * p_home_dir,
                   const char * p_file_path,
                   const char * p_read_mode);


// HEADER GUARD
#ifdef __cplusplus
}
#endif // END __cplusplus
#endif //BSLE_GALINDEZ_INCLUDE_SERVER_FILE_API_H_
