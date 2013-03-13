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

// -1 == infinite
#define TCP_MAX_RECONNECTS 2
#define TCP_MAX_TIMEOUTS -1
#define TCP_MAX_CONSECUTIVE_TIMEOUTS -1
#define TCP_MAX_READ_BYTES 1024*1024*5

#define TCP_TCPRUNSTATUS_SIZE 27

enum TcpRunStatus {
    NOOP,               // 0
    FORCED,             // 1
    MULTI_FAIL,         // 2
    GREAT_SUCCESS,      // 3
    WTF,                // 4
    SOCKET_FAIL,        // 5
    SOCKET_OK,          // 6
    GETHOSTBYNAME_FAIL, // 7
    GETHOSTBYNAME_OK,   // 8
    CONNECT_FAIL,       // 9
    CONNECT_OK,         // 10
    CONNECT_TIMEOUT,    // 11
    WRITE_FAIL,         // 12
    WRITE_OK,           // 13
    SELECT_FAIL,        // 14
    SELECT_TIMEOUT,     // 15
    SELECT_OOB,         // 16
    SELECT_OK,          // 17
    RECV_FAIL,          // 18
    RECV_DONE,          // 19
    RECV_OK,            // 20
    RECV_TIMEOUT,       // 21
    RECV_RESET,         // 22
    SHUTDOWN_FAIL,      // 23
    SHUTDOWN_OK,        // 24
    CLOSE_FAIL,         // 25
    CLOSE_OK            // 26
};

extern const char* TcpRunStatusStrings [TCP_TCPRUNSTATUS_SIZE];

bool resolve_host_name (sockaddr_in*, const char*, int);

class TcpRun
{
    private:
    unsigned int id;

    // test input
    //const char* host_name;
    //const int port_number;
    const sockaddr_in* target;
    const CharBuffer* request;

    unsigned int max_reconnects;
    unsigned int max_timeouts;
    unsigned int max_consecutive_timeouts;
    unsigned long max_read_bytes;

    // a read buffer
    CharBuffer buff;

    // timeout values
    struct timeval select_timeout;
    struct timeval recv_timeout;

    // socket/net stuff
    int socket_id;
    //struct hostent* host;
    fd_set socket_set_read;
    fd_set socket_set_except;

    // counters
    unsigned int connect_attempts;
    unsigned int connect_successes;
    unsigned int timeouts;
    unsigned int consecutive_timeouts;
    unsigned long bytes_written;
    unsigned long bytes_read;
    unsigned int premature_shutdowns;

    // local variables
    // no need to allocate these on each function call (potentially called thousands of times), might as well have it all before-hand
    // "s_" prefix means "status" (return value) of [suffix]()
    int retval;

    TcpRunStatus s_go;

    TcpRunStatus s_go_init;
    TcpRunStatus s_go_connect;
    TcpRunStatus s_go_write;
    TcpRunStatus s_go_read;
    TcpRunStatus s_go_disconnect;

    TcpRunStatus s_do_socket;
    TcpRunStatus s_do_connect;
    TcpRunStatus s_do_write;
    TcpRunStatus s_do_select;
    TcpRunStatus s_do_recv;
    TcpRunStatus s_do_shutdown;
    TcpRunStatus s_do_close;

    TcpRunStatus s_premature;

    TcpRunStatus s_do_select_last;  // the *last* status from do_select()
    TcpRunStatus s_do_recv_last;    // the *last* status from do_recv()

    public:
    TcpRun (unsigned int, sockaddr_in*, const CharBuffer*);

    void set_max_reconnects (unsigned int);
    void set_max_timeouts (unsigned int);
    void set_max_consecutive_timeouts (unsigned int);
    void set_max_read_bytes (unsigned long);

    TcpRunStatus go ();

    unsigned int get_connect_attempts();
    unsigned int get_connect_successes();
    unsigned int get_timeouts ();
    unsigned long get_bytes_written ();
    unsigned long get_bytes_read ();
    unsigned int get_premature_shutdowns ();

    void print ();

    private:
    // mid-level
    TcpRunStatus go_init ();
    TcpRunStatus go_connect ();
    TcpRunStatus go_write ();
    TcpRunStatus go_read ();
    TcpRunStatus go_disconnect ();

    // low-level
    TcpRunStatus do_socket ();
    //TcpRunStatus do_gethostbyname ();
    TcpRunStatus do_connect ();
    TcpRunStatus do_write ();
    TcpRunStatus do_select ();
    TcpRunStatus do_recv (bool);
    TcpRunStatus do_shutdown ();
    TcpRunStatus do_close ();

    void sanity (const char*);
};

#endif
