#ifndef _TCP_H_
#define _TCP_H_

#include "utils.h"
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#define TCP_SELECT_TIMEOUT_SEC 5
#define TCP_SELECT_TIMEOUT_USEC 0

#define TCP_RECV_TIMEOUT_SEC 2
#define TCP_RECV_TIMEOUT_USEC 0

#define TCP_MAX_RECONNECTS 0
#define TCP_MAX_READ_TIMEOUTS -1

enum TcpRunStatus {
    NOOP,
    FORCED,
    MULTI_FAIL,
    GREAT_SUCCESS,
    SOCKET_FAIL,
    SOCKET_OK,
    GETHOSTBYNAME_FAIL,
    GETHOSTBYNAME_OK,
    CONNECT_FAIL,
    CONNECT_OK,
    CONNECT_TIMEOUT,
    WRITE_FAIL,
    WRITE_OK,
    SELECT_FAIL,
    SELECT_TIMEOUT,
    SELECT_OK,
    RECV_FAIL,
    RECV_DONE,
    RECV_OK,
    RECV_TIMEOUT,
    RECV_RESET,
    SHUTDOWN_FAIL,
    SHUTDOWN_OK,
    CLOSE_FAIL,
    CLOSE_OK
};

void print_trs (TcpRunStatus trs);

bool resolve_host_name (sockaddr_in*, const char*, int);

class TcpRun
{
    private:
    // test input
    //const char* host_name;
    //const int port_number;
    const sockaddr_in* target;
    const CharBuffer* request;

    // a read buffer
    CharBuffer buff;

    // timeout values
    struct timeval select_timeout;
    struct timeval recv_timeout;

    // socket/net stuff
    int socket_id;
    //struct hostent* host;
    fd_set socket_set;

    // counters
    unsigned int timeouts;
    //unsigned int connect_timeouts;
    unsigned int resets;
    unsigned long bytes_written;
    unsigned long bytes_read;
    unsigned int premature_shutdowns;

    public:
    //TcpRun (const char*, const int, const CharBuffer*);
    TcpRun (sockaddr_in*, const CharBuffer*);

    TcpRunStatus go ();

    unsigned int get_timeouts ();
    //unsigned int get_connect_timeouts ();
    unsigned int get_resets ();
    unsigned long get_bytes_written ();
    unsigned long get_bytes_read ();
    unsigned int get_premature_shutdowns ();

    void print ();

    private:
    // mid-level
    TcpRunStatus _go_init ();
    TcpRunStatus _go_connect (int);
    TcpRunStatus _go_write ();
    TcpRunStatus _go_read (int);
    TcpRunStatus _go_disconnect ();

    // low-level
    TcpRunStatus do_socket ();
    //TcpRunStatus do_gethostbyname ();
    TcpRunStatus do_connect ();
    TcpRunStatus do_write ();
    TcpRunStatus do_select ();
    TcpRunStatus do_recv ();
    TcpRunStatus do_shutdown ();
    TcpRunStatus do_close ();
};

#endif
