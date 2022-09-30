#ifndef BSLE_GALINDEZ_SRC_SERVER_SERVER_SOCK_H_
#define BSLE_GALINDEZ_SRC_SERVER_SERVER_SOCK_H_

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


void start_server(uint32_t port_num);


#ifdef __cplusplus
}
#endif

#endif //BSLE_GALINDEZ_SRC_SERVER_SERVER_SOCK_H_
