#ifndef _TCP_H_
#define _TCP_H_

#include "utils.h"
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#define TCP_CONNECTION_TIMEOUT_SEC 3
#define TCP_CONNECTION_TIMEOUT_USEC 0

class TcpConnection
{
    private:
    bool connected;
    bool timeout;
    int socket_id;
    fd_set socket_set;
    struct timeval wait_timeout;
    CharBuffer buffer;

    ////////////////////////////////////////////////////////////////////////////////

    private:
    static int _open_tcp_socket (const char *host_name, const int port_number);

    bool _is_read_ready ();

    public:
    TcpConnection ();
    void connect (const char *host_name, const int port_number);
    void disconnect ();
    void shutdown_writes ();
    void shutdown_reads ();

    int write (const char *message, const int message_length);
    int write (CharBuffer &b);
    int swrite (const char *message);
    int swrite (string &message);

    bool check_timeout ();

    int read (char *buffer, const int buffer_size);
    int read (CharBuffer &b);
    int read ();

    int sread (char *buffer, const int buffer_size);
    int sread (CharBuffer &b);
    int sread ();

    int readline (char *buffer, const int buffer_size);
    int readline (CharBuffer &b);
    int readline ();

    int read_all (unsigned long* num_bytes = NULL);
    int print_all (unsigned long* num_bytes = NULL);

    const char* last_read ();
};

#endif
