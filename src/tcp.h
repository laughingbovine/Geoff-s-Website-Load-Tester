#ifndef _TCP_H_
#define _TCP_H_

#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>

#define TCP_CONNECTION_BUFFER_SIZE 5120
#define TCP_CONNECTION_TIMEOUT_SEC 3
#define TCP_CONNECTION_TIMEOUT_USEC 0

struct tcp_connection {
    int socket_id;
    fd_set socket_set;
    struct timeval wait_timeout;
    int wait_timeout_sec;
    int wait_timeout_usec;
    char* buffer;
    int buffer_size;
};
typedef struct tcp_connection tcp_connection;

tcp_connection* new_tcp_connection_object ();
void connect_tcp (tcp_connection* this, const char *host_name, const int port_number);
void disconnect_tcp (tcp_connection* this);
void free_tcp (tcp_connection* this);

int swrite_tcp (tcp_connection* this, const char* message);
int sread_tcp (tcp_connection* this);
int sreadline_tcp (tcp_connection* this);
int wait_tcpread (tcp_connection* this);
int readall_tcp (tcp_connection* this, unsigned long* num_bytes);

#endif
