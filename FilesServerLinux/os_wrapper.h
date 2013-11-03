#ifndef OS_WRAPPER_H_
#define OS_WRAPPER_H_

#include "common.h"

int init_sockets();

int new_socket();

int close_socket(int sockfd);

int connect_to_server(int server_socket, char *ip, int port);

int build_server(int port, int slots);

int my_send(int socket, const char *buffer, int max_size, int flags);

int my_recv(int socket, char *buffer, int max_size, int flags);

#endif /* OS_WRAPPER_H_ */
