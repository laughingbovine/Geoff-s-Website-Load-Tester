#ifndef _TCP_H_
#define _TCP_H_

#include "utils.h"
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#define TCP_CONNECTION_TIMEOUT_SEC 30
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
    static int _open_tcp_socket (const char*, const int);

    bool _is_read_ready ();

    int _get (char*, const int, const int);
    int _get_until (char*, const int, const char, const int);

    public:
    TcpConnection ();
    int connect (const char*, const int);
    void disconnect ();
    void shutdown_writes ();
    void shutdown_reads ();

    int write (const char*, const int);
    int write (CharBuffer&);
    int swrite (const char*);
    int swrite (string&);

    bool check_timeout ();

    int read (char*, const int);
    int read (CharBuffer&);
    int read ();

    int sread (char*, const int);
    int sread (CharBuffer&);
    int sread ();

    int readline (char*, const int);
    int readline (CharBuffer&);
    int readline ();

    int read_all (unsigned long*);
    int print_all (unsigned long*);

    const char* last_read ();
};

#endif
