#ifndef BSLE_GALINDEZ_SRC_SERVER_SERVER_SOCK_H_
#define BSLE_GALINDEZ_SRC_SERVER_SERVER_SOCK_H_

#include "server_db.h"
#ifdef __cplusplus
extern "C" {
#endif

#include <netdb.h>
#include <sys/socket.h>
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <thread_pool.h>
#include <utils.h>
#include <server.h>
#include <server_ctrl.h>


void start_server(db_t * p_db, uint32_t port_num, uint32_t timeout);


#ifdef __cplusplus
}
#endif

#endif //BSLE_GALINDEZ_SRC_SERVER_SERVER_SOCK_H_
