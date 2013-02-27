#include "tcp.h"

const char* TcpRunStatusStrings [TCP_TCPRUNSTATUS_SIZE] = {
    "NOOP",
    "FORCED",
    "MULTI_FAIL",
    "GREAT_SUCCESS",
    "WTF",
    "SOCKET_FAIL",
    "SOCKET_OK",
    "GETHOSTBYNAME_FAIL",
    "GETHOSTBYNAME_OK",
    "CONNECT_FAIL",
    "CONNECT_OK",
    "CONNECT_TIMEOUT",
    "WRITE_FAIL",
    "WRITE_OK",
    "SELECT_FAIL",
    "SELECT_TIMEOUT",
    "SELECT_OK",
    "RECV_FAIL",
    "RECV_DONE",
    "RECV_OK",
    "RECV_TIMEOUT",
    "RECV_RESET",
    "SHUTDOWN_FAIL",
    "SHUTDOWN_OK",
    "CLOSE_FAIL",
    "CLOSE_OK"
};

////////////////////////////////////////////////////////////////////////////////

bool resolve_host_name (sockaddr_in* ret, const char* host_name, int port_number)
{
    hostent* host = gethostbyname(host_name);

    if (host == NULL)
    {
        printf("gethostbyname('%s') FAIL: [%d]%s\n", host_name, h_errno, hstrerror(h_errno));
        return false;
    }

    // zero the server address object
    bzero((char*)ret, sizeof(&ret));
    // (as in do_socket()) server is on the internet
    ret->sin_family = AF_INET;
    // copy server address string over to object
    bcopy((char*)host->h_addr, (char*)&ret->sin_addr.s_addr, host->h_length);
    // convert port number
    ret->sin_port = htons(port_number);

    //struct sockaddr_in server_address;

    //// zero the server address object
    //bzero((char*)&server_address, sizeof(server_address));
    //// (as in do_socket()) server is on the internet
    //server_address.sin_family = AF_INET;
    //// copy server address string over to object
    //bcopy((char*)host->h_addr, (char*)&server_address.sin_addr.s_addr, host->h_length);
    //// convert port number
    //server_address.sin_port = htons(port_number);

    return true;
}

////////////////////////////////////////////////////////////////////////////////

TcpRun::TcpRun (unsigned int id, sockaddr_in* target, const CharBuffer* request) :
    id(id),
    target(target),
    request(request),
    max_reconnects(TCP_MAX_RECONNECTS),
    max_timeouts(TCP_MAX_TIMEOUTS),
    max_consecutive_timeouts(TCP_MAX_CONSECUTIVE_TIMEOUTS),
    max_read_bytes(TCP_MAX_READ_BYTES),
    connect_attempts(0),
    connect_successes(0),
    timeouts(0),
    consecutive_timeouts(0),
    bytes_written(0),
    bytes_read(0),
    premature_shutdowns(0),
    s_go(NOOP),
    s_go_init(NOOP),
    s_go_connect(NOOP),
    s_go_write(NOOP),
    s_go_read(NOOP),
    s_go_disconnect(NOOP),
    s_do_socket(NOOP),
    s_do_connect(NOOP),
    s_do_write(NOOP),
    s_do_select(NOOP),
    s_do_recv(NOOP),
    s_do_shutdown(NOOP),
    s_do_close(NOOP),
    s_premature(NOOP),
    s_do_select_last(NOOP),
    s_do_recv_last(NOOP)
{}

void TcpRun::set_max_reconnects (unsigned int max)
{
    max_reconnects = max;
}

void TcpRun::set_max_timeouts (unsigned int max)
{
    max_timeouts = max;
}

void TcpRun::set_max_consecutive_timeouts (unsigned int max)
{
    max_consecutive_timeouts = max;
}

void TcpRun::set_max_read_bytes (unsigned long max)
{
    max_read_bytes = max;
}

// low-level wrappers around socket i/o functions
////////////////////////////////////////////////////////////////////////////////

TcpRunStatus TcpRun::do_socket ()
{
    // AF_INET means that this socket is connecting to an internet address (AF_UNIX would be a unix socket instead)
    // SOCK_STREAM basically means "use TCP" (with SOCK_DGRAM for "use UDP")
    // 0 is for protocol, but should always be 0 which means it will choose the "most appropriate" protocol (TCP for STREAMs and UDP for DataGRAMs)
    socket_id = socket(AF_INET, SOCK_STREAM, 0);

    return socket_id >= 0 ? SOCKET_OK : SOCKET_FAIL;
}

//TcpRunStatus TcpRun::do_gethostbyname ()
//{
//    host = gethostbyname(host_name);
//
//    return host != NULL ? GETHOSTBYNAME_OK : GETHOSTBYNAME_FAIL;
//}

TcpRunStatus TcpRun::do_connect ()
{
    //struct sockaddr_in server_address;

    //// zero the server address object
    //bzero((char*)&server_address, sizeof(server_address));
    //// (as in do_socket()) server is on the internet
    //server_address.sin_family = AF_INET;
    //// copy server address string over to object
    //bcopy((char*)host->h_addr, (char*)&server_address.sin_addr.s_addr, host->h_length);
    //// convert port number
    //server_address.sin_port = htons(port_number);

    // open connection
    if (connect(socket_id, (struct sockaddr*)target, sizeof(sockaddr_in)) == 0)
        return CONNECT_OK;
    else if (errno == 60)
        return CONNECT_TIMEOUT;
    else
        return CONNECT_FAIL;
}

TcpRunStatus TcpRun::do_write ()
{
    retval = write(socket_id, request->chars, request->size);

    if (retval > 0)
        bytes_written += retval;

    return retval == request->size ? WRITE_OK : WRITE_FAIL;
}

TcpRunStatus TcpRun::do_select ()
{
    // initialize timeout
    select_timeout.tv_sec   = TCP_SELECT_TIMEOUT_SEC;
    select_timeout.tv_usec  = TCP_SELECT_TIMEOUT_USEC;

    // select() uses the fd_set struct to manage select()ing multiple file descriptors
    // unfortunately we have to use this even though we just want to check on the one
    FD_ZERO(&socket_set); // reset set
    FD_SET(socket_id, &socket_set); // set our fd (socket)

    retval = select(FD_SETSIZE, &socket_set, NULL, NULL, &select_timeout);

    if (retval > 0)
        return SELECT_OK;
    else if (retval == 0)
        return SELECT_TIMEOUT;
    else
        return SELECT_FAIL;
}

TcpRunStatus TcpRun::do_recv ()
{
    // initialize timeout
    recv_timeout.tv_sec     = TCP_RECV_TIMEOUT_SEC;
    recv_timeout.tv_usec    = TCP_RECV_TIMEOUT_USEC;

    // set timeout on socket
    setsockopt(socket_id, SOL_SOCKET, SO_RCVTIMEO, (void*)&recv_timeout, sizeof(struct timeval));

    // grab input from socket
    if (max_read_bytes >= 0)
    {
        if (bytes_read >= max_read_bytes)
            retval = 0; // done
        else if (buff.size > (max_read_bytes - bytes_read))
            retval = recv(socket_id, buff.chars, max_read_bytes - bytes_read, 0);
        else
            retval = recv(socket_id, buff.chars, buff.size, 0);
    }
    else
    {
        retval = recv(socket_id, buff.chars, buff.size, 0);
    }

    if (retval > 0)
        bytes_read += retval;

    if (retval > 0)
        return RECV_OK;
    else if (retval == 0)
        return RECV_DONE;
    else if (errno == 60)
        return RECV_TIMEOUT;
    else if (errno == 54)
        return RECV_RESET;
    else
        return RECV_FAIL;
}

TcpRunStatus TcpRun::do_shutdown ()
{
    if (shutdown(socket_id, SHUT_RDWR) == 0) {
        return SHUTDOWN_OK;
    } else {
        if (errno == 57) {
            // socket already shut down (remotely?)
            return SHUTDOWN_OK;
        } else {
            return SHUTDOWN_FAIL;
        }
    }
}

TcpRunStatus TcpRun::do_close ()
{
    return close(socket_id) == 0 ? CLOSE_OK : CLOSE_FAIL;
}

// higher-level wrappers around above
// these will be displaying error messages
////////////////////////////////////////////////////////////////////////////////

// a wrapper around do_socket()
TcpRunStatus TcpRun::go_init ()
{
    s_do_socket = NOOP;

    s_do_socket = do_socket();

    sanity("go_init");

    //if (s_do_socket != SOCKET_OK)
    //    printf("[%5u] TcpRun::go_init(): do_socket() FAIL: [%d]%s\n", id, errno, strerror(errno));

    return s_do_socket;
}

// a wrapper around do_connect()
TcpRunStatus TcpRun::go_connect ()
{
    s_do_connect = NOOP;

    while (true)
    {
        connect_attempts++;
        s_do_connect = do_connect();

        sanity("go_connect");

        if (s_do_connect != CONNECT_OK && max_reconnects >= 0 && connect_attempts > max_reconnects) {
            //printf("[%5u] TcpRun::go_connect(): do_connect() FAIL: [%d]%s\n", id, errno, strerror(errno));
            break;
        } else if (s_do_connect == CONNECT_OK) {
            connect_successes++;
            break;
        }
    }

    return s_do_connect;
}

// simple wrapper around do_write()
TcpRunStatus TcpRun::go_write ()
{
    s_do_write = NOOP;

    s_do_write = do_write();

    sanity("go_write");

    //if (s_do_write != WRITE_OK) {
    //    printf("[%5u] TcpRun::go_write(): do_write() FAIL: [%d]%s\n", id, errno, strerror(errno));
    //}

    return s_do_write;
}

// preforms select() and recv() until there's no more data and retries on timeouts
TcpRunStatus TcpRun::go_read ()
{
    s_premature         = NOOP;
    s_do_select         = NOOP;
    s_do_recv           = NOOP;

    s_do_select_last    = NOOP;
    s_do_recv_last      = NOOP;

    while (s_do_recv != RECV_DONE) {
        s_do_select_last = s_do_select;
        s_do_recv_last = s_do_recv;

        if (Global::premature_shutdown) {
            premature_shutdowns++;
            s_premature = FORCED;
            //printf("[%5u] TcpRun::go_read(): premature_shutdown Before select\n", id);
            break;
        }

        s_do_select = do_select();

        if (Global::premature_shutdown) {
            premature_shutdowns++;
            s_premature = FORCED;
            //printf("[%5u] TcpRun::go_read(): premature_shutdown After select\n", id);
            break;
        }

        if (s_do_select == SELECT_OK) {
            s_do_recv = do_recv();

            if (s_do_recv == RECV_TIMEOUT) {
                timeouts++;

                if (s_do_recv_last == RECV_TIMEOUT) {
                    consecutive_timeouts++;
                } else {
                    consecutive_timeouts = 0;
                }

                if (max_consecutive_timeouts >= 0 && consecutive_timeouts > max_consecutive_timeouts) {
                    //printf("[%5u] TcpRun::go_read(): do_recv() FAIL: too many consecutive timeouts\n", id);
                    break;
                }

                if (max_timeouts >= 0 && timeouts > max_timeouts) {
                    //printf("[%5u] TcpRun::go_read(): do_recv() FAIL: too many timeouts\n", id);
                    break;
                }
            } else if (s_do_recv == RECV_RESET) {
                //printf("[%5u] TcpRun::go_read(): do_recv() RESET: [%d]%s\n", id, errno, strerror(errno));
                break;
            } else if (s_do_recv == RECV_FAIL) {
                //printf("[%5u] TcpRun::go_read(): do_recv() FAIL: [%d]%s\n", id, errno, strerror(errno));
                break;
            }
        } else if (s_do_select == SELECT_TIMEOUT) {
            timeouts++;

            if (s_do_select_last == SELECT_TIMEOUT) {
                consecutive_timeouts++;
            } else {
                consecutive_timeouts = 0;
            }

            if (max_consecutive_timeouts >= 0 && consecutive_timeouts > max_consecutive_timeouts) {
                //printf("[%5u] TcpRun::go_read(): do_select() FAIL: too many consecutive timeouts\n", id);
                break;
            }

            if (max_timeouts >= 0 && timeouts > max_timeouts) {
                //printf("[%5u] TcpRun::go_read(): do_select() FAIL: too many timeouts\n", id);
                break;
            }
        } else if (s_do_select == SELECT_FAIL) {
            //printf("[%5u] TcpRun::go_read(): do_select() FAIL: [%d]%s\n", id, errno, strerror(errno));
            break;
        }
    }

    return s_premature == NOOP ? (s_do_select == SELECT_OK ? s_do_recv : s_do_select) : s_premature;

    // so... possible return values are:
    //
    // NOOP             (will never return this)
    // SELECT_OK        (will never return this)
    // RECV_OK          (will never return this)
    //
    // SELECT_TIMEOUT   maximum number of timeouts reached, last one on select()
    // RECV_TIMEOUT     maximum number of timeouts reached, last one on recv()
    // RECV_RESET       server reset the connection
    // FORCED           premature shutdown
    // SELECT_FAIL      select() failed for some other reason
    // RECV_FAIL        recv() failed for some other reason
    // RECV_DONE        successful read
}

// the counterpart to a successful go_connect()
TcpRunStatus TcpRun::go_disconnect ()
{
    s_do_shutdown   = NOOP;
    s_do_close      = NOOP;

    s_do_shutdown = do_shutdown();

    //if (s_do_shutdown == SHUTDOWN_FAIL) {
    //    printf("[%5u] TcpRun::go_disconnect(): do_shutdown() FAIL: [%d]%s\n", id, errno, strerror(errno));
    //}

    s_do_close = do_close();

    //if (s_do_close == CLOSE_FAIL) {
    //    printf("[%5u] TcpRun::go_disconnect(): do_close() FAIL: [%d]%s\n", id, errno, strerror(errno));
    //}

    return s_do_shutdown == SHUTDOWN_OK ? s_do_close : s_do_shutdown;

    // so... possible return values are:
    //
    // SHUTDOWN_OK      (will never return this)
    //
    // SHUTDOWN_FAIL    shutdown() failed for some reason
    // CLOSE_FAIL       close() failed for some reason
    // CLOSE_OK         disconnect successful
}

////////////////////////////////////////////////////////////////////////////////

TcpRunStatus TcpRun::go ()
{
    s_go            = NOOP;
    s_go_init       = NOOP;
    s_go_connect    = NOOP;
    s_go_write      = NOOP;
    s_go_read       = NOOP;
    s_go_disconnect = NOOP;

    connect_attempts        = 0;
    connect_successes       = 0;
    timeouts                = 0;
    consecutive_timeouts    = 0;
    bytes_written           = 0;
    bytes_read              = 0;
    premature_shutdowns     = 0;

    sanity("go 1");

    // do the run
    s_go_init = go_init();
    if (s_go_init == SOCKET_OK) {
        s_go_connect = go_connect();
        if (s_go_connect == CONNECT_OK) {
            s_go_write = go_write();
            if (s_go_write == WRITE_OK) {
                s_go_read = go_read();
            }
            s_go_disconnect = go_disconnect();
        }
    }

    sanity("go 2");

    // come up with the return value
    if (s_go_init == SOCKET_OK) {
        if (s_go_connect == CONNECT_OK) {
            if (s_go_disconnect == CLOSE_OK) {
                if (s_go_write == WRITE_OK) {
                    if (s_go_read == RECV_DONE) {
                        s_go = GREAT_SUCCESS;
                    } else {
                        s_go = s_go_read;
                    }
                } else {
                    s_go = s_go_write;
                }
            } else {
                if (s_go_write == WRITE_OK && s_go_read == RECV_DONE) {
                    s_go = s_go_disconnect;
                } else {
                    s_go = MULTI_FAIL;
                }
            }
        } else {
            s_go = s_go_connect;
        }
    } else {
        s_go = s_go_init;
    }

    sanity("go 3");

    // check for insanity
    if (s_go < 0 || s_go >= TCP_TCPRUNSTATUS_SIZE) { // TODO: remove after we figure out this SIGSEGV
        //printf("[%5u] TcpRun::go(): WTF - g:%12d i:%12d c:%12d w:%12d r:%12d d:%12d\n", id, s_go, s_go_init, s_go_connect, s_go_write, s_go_read, s_go_disconnect);
        return WTF;
    } else {
        return s_go;
    }
}

unsigned int TcpRun::get_connect_attempts ()
{
    return connect_attempts;
}

unsigned int TcpRun::get_connect_successes ()
{
    return connect_successes;
}

unsigned int TcpRun::get_timeouts ()
{
    return timeouts;
}

unsigned long TcpRun::get_bytes_written ()
{
    return bytes_written;
}

unsigned long TcpRun::get_bytes_read ()
{
    return bytes_read;
}

unsigned int TcpRun::get_premature_shutdowns ()
{
    return premature_shutdowns;
}

void TcpRun::print ()
{
    printf("%s", request->chars);
}

void TcpRun::sanity (const char* label)
{
    bool sane = true;

    if (s_go < 0 || s_go >= TCP_TCPRUNSTATUS_SIZE
        || s_go_init < 0 || s_go_init >= TCP_TCPRUNSTATUS_SIZE
        || s_go_connect < 0 || s_go_connect >= TCP_TCPRUNSTATUS_SIZE
        || s_go_write < 0 || s_go_write >= TCP_TCPRUNSTATUS_SIZE
        || s_go_read < 0 || s_go_read >= TCP_TCPRUNSTATUS_SIZE
        || s_go_disconnect < 0 || s_go_disconnect >= TCP_TCPRUNSTATUS_SIZE
        || s_do_socket < 0 || s_do_socket >= TCP_TCPRUNSTATUS_SIZE
        || s_do_connect < 0 || s_do_connect >= TCP_TCPRUNSTATUS_SIZE
        || s_do_write < 0 || s_do_write >= TCP_TCPRUNSTATUS_SIZE
        || s_do_select < 0 || s_do_select >= TCP_TCPRUNSTATUS_SIZE
        || s_do_recv < 0 || s_do_recv >= TCP_TCPRUNSTATUS_SIZE
        || s_do_shutdown < 0 || s_do_shutdown >= TCP_TCPRUNSTATUS_SIZE
        || s_do_close < 0 || s_do_close >= TCP_TCPRUNSTATUS_SIZE
        || s_premature < 0 || s_premature >= TCP_TCPRUNSTATUS_SIZE
        || s_do_select_last < 0 || s_do_select_last >= TCP_TCPRUNSTATUS_SIZE
        || s_do_recv_last < 0 || s_do_recv_last >= TCP_TCPRUNSTATUS_SIZE
    ) {
        sane = false;
    }

    if (!sane) {
        printf("[%5u] %10s g:%11d gi:%11d gc:%11d gw:%11d gr:%11d gd:%11d ds:%11d dc:%11d dw:%11d ds:%11d dr:%11d ds:%11d dc:%11d p:%11d dsl:%11d drl:%11d\n", id, label, s_go, s_go_init, s_go_connect, s_go_write, s_go_read, s_go_disconnect, s_do_socket, s_do_connect, s_do_write, s_do_select, s_do_recv, s_do_shutdown, s_do_close, s_premature, s_do_select_last, s_do_recv_last);

        //printf("[%5u] TcpRun::sanity(): WTF - %s ", id, label);

        //if (s_go < 0 || s_go >= TCP_TCPRUNSTATUS_SIZE)
        //    printf("s_go:%d ", s_go);
        //if (s_go_init < 0 || s_go_init >= TCP_TCPRUNSTATUS_SIZE)
        //    printf("s_go_init:%d ", s_go_init);
        //if (s_go_connect < 0 || s_go_connect >= TCP_TCPRUNSTATUS_SIZE)
        //    printf("s_go_connect:%d ", s_go_connect);
        //if (s_go_write < 0 || s_go_write >= TCP_TCPRUNSTATUS_SIZE)
        //    printf("s_go_write:%d ", s_go_write);
        //if (s_go_read < 0 || s_go_read >= TCP_TCPRUNSTATUS_SIZE)
        //    printf("s_go_read:%d ", s_go_read);
        //if (s_go_disconnect < 0 || s_go_disconnect >= TCP_TCPRUNSTATUS_SIZE)
        //    printf("s_go_disconnect:%d ", s_go_disconnect);
        //if (s_do_socket < 0 || s_do_socket >= TCP_TCPRUNSTATUS_SIZE)
        //    printf("s_do_socket:%d ", s_do_socket);
        //if (s_do_connect < 0 || s_do_connect >= TCP_TCPRUNSTATUS_SIZE)
        //    printf("s_do_connect:%d ", s_do_connect);
        //if (s_do_write < 0 || s_do_write >= TCP_TCPRUNSTATUS_SIZE)
        //    printf("s_do_write:%d ", s_do_write);
        //if (s_do_select < 0 || s_do_select >= TCP_TCPRUNSTATUS_SIZE)
        //    printf("s_do_select:%d ", s_do_select);
        //if (s_do_recv < 0 || s_do_recv >= TCP_TCPRUNSTATUS_SIZE)
        //    printf("s_do_recv:%d ", s_do_recv);
        //if (s_do_shutdown < 0 || s_do_shutdown >= TCP_TCPRUNSTATUS_SIZE)
        //    printf("s_do_shutdown:%d ", s_do_shutdown);
        //if (s_do_close < 0 || s_do_close >= TCP_TCPRUNSTATUS_SIZE)
        //    printf("s_do_close:%d ", s_do_close);
        //if (s_premature < 0 || s_premature >= TCP_TCPRUNSTATUS_SIZE)
        //    printf("s_premature:%d ", s_premature);
        //if (s_do_select_last < 0 || s_do_select_last >= TCP_TCPRUNSTATUS_SIZE)
        //    printf("s_do_select_last:%d ", s_do_select_last);
        //if (s_do_recv_last < 0 || s_do_recv_last >= TCP_TCPRUNSTATUS_SIZE)
        //    printf("s_do_recv_last:%d ", s_do_recv_last);

        //printf("\n");
    }
}
