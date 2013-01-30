#include "tcp.h"

////////////////////////////////////////////////////////////////////////////////

void print_trs (TcpRunStatus trs)
{
    switch (trs)
    {
        case NOOP:
            printf("NOOP\n"); break;
        case FORCED:
            printf("FORCED\n"); break;
        case MULTI_FAIL:
            printf("MULTI_FAIL\n"); break;
        case GREAT_SUCCESS:
            printf("GREAT_SUCCESS\n"); break;
        case SOCKET_FAIL:
            printf("SOCKET_FAIL\n"); break;
        case SOCKET_OK:
            printf("SOCKET_OK\n"); break;
        case GETHOSTBYNAME_FAIL:
            printf("GETHOSTBYNAME_FAIL\n"); break;
        case GETHOSTBYNAME_OK:
            printf("GETHOSTBYNAME_OK\n"); break;
        case CONNECT_FAIL:
            printf("CONNECT_FAIL\n"); break;
        case CONNECT_OK:
            printf("CONNECT_OK\n"); break;
        case CONNECT_TIMEOUT:
            printf("CONNECT_TIMEOUT\n"); break;
        case WRITE_FAIL:
            printf("WRITE_FAIL\n"); break;
        case WRITE_OK:
            printf("WRITE_OK\n"); break;
        case SELECT_FAIL:
            printf("SELECT_FAIL\n"); break;
        case SELECT_TIMEOUT:
            printf("SELECT_TIMEOUT\n"); break;
        case SELECT_OK:
            printf("SELECT_OK\n"); break;
        case RECV_FAIL:
            printf("RECV_FAIL\n"); break;
        case RECV_DONE:
            printf("RECV_DONE\n"); break;
        case RECV_OK:
            printf("RECV_OK\n"); break;
        case RECV_TIMEOUT:
            printf("RECV_TIMEOUT\n"); break;
        case RECV_RESET:
            printf("RECV_RESET\n"); break;
        case SHUTDOWN_FAIL:
            printf("SHUTDOWN_FAIL\n"); break;
        case SHUTDOWN_OK:
            printf("SHUTDOWN_OK\n"); break;
        case CLOSE_FAIL:
            printf("CLOSE_FAIL\n"); break;
        case CLOSE_OK:
            printf("CLOSE_OK\n"); break;
    }
}

bool resolve_host_name (sockaddr_in* ret, const char* host_name, int port_number)
{
    hostent* host = gethostbyname(host_name);

    if (host == NULL)
    {
        warn("gethostbyname('%s') FAIL: [%d]%s", host_name, h_errno, hstrerror(h_errno));
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

TcpRun::TcpRun (sockaddr_in* target, const CharBuffer* request) :
    target(target),
    request(request),
    max_reconnects(TCP_MAX_RECONNECTS),
    max_timeouts(TCP_MAX_TIMEOUTS),
    max_resets(TCP_MAX_RESETS),
    max_read_bytes(TCP_MAX_READ_BYTES),
    connect_attempts(0),
    timeouts(0),
    resets(0),
    bytes_written(0),
    bytes_read(0),
    premature_shutdowns(0)
{}

void TcpRun::set_max_reconnects (unsigned int max)
{
    max_reconnects = max;
}

void TcpRun::set_max_timeouts (unsigned int max)
{
    max_timeouts = max;
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
    int r = write(socket_id, request->chars, request->size);

    if (r > 0)
        bytes_written += r;

    return r == request->size ? WRITE_OK : WRITE_FAIL;
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

    int r = select(FD_SETSIZE, &socket_set, NULL, NULL, &select_timeout);

    if (r > 0)
        return SELECT_OK;
    else if (r == 0)
        return SELECT_TIMEOUT;
    else
        return SELECT_FAIL;
}

TcpRunStatus TcpRun::do_recv ()
{
    // return val for recv()
    int r;

    // initialize timeout
    recv_timeout.tv_sec     = TCP_RECV_TIMEOUT_SEC;
    recv_timeout.tv_usec    = TCP_RECV_TIMEOUT_USEC;

    // set timeout on socket
    setsockopt(socket_id, SOL_SOCKET, SO_RCVTIMEO, (void*)&recv_timeout, sizeof(struct timeval));

    // grab input from socket
    if (max_read_bytes >= 0)
    {
        if (bytes_read >= max_read_bytes)
            r = 0; // done
        else if (bytes_read + buff.size > max_read_bytes)
            r = recv(socket_id, buff.chars, max_read_bytes - bytes_read, 0);
        else
            r = recv(socket_id, buff.chars, buff.size, 0);
    }
    else
    {
        r = recv(socket_id, buff.chars, buff.size, 0);
    }

    if (r > 0)
        bytes_read += r;

    if (r > 0)
        return RECV_OK;
    else if (r == 0)
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
    if (shutdown(socket_id, SHUT_RDWR) == 0)
    {
        return SHUTDOWN_OK;
    }
    else
    {
        if (errno == 57) // socket already shut down (remotely?)
        {
            return SHUTDOWN_OK;
        }
        else
        {
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
TcpRunStatus TcpRun::_go_init ()
{
    TcpRunStatus socket = NOOP;
    //TcpRunStatus gethostbyname = NOOP;

    socket = do_socket();

    if (socket != SOCKET_OK)
        warn("go(): do_socket() FAIL: [%d]%s", errno, strerror(errno));

    return socket;
}

// a wrapper around do_connect()
TcpRunStatus TcpRun::_go_connect ()
{
    TcpRunStatus connect = NOOP;

    while (connect != CONNECT_OK)
    {
        connect = do_connect();

        connect_attempts++;

        if (connect != CONNECT_OK && max_reconnects >= 0 && connect_attempts > max_reconnects)
        {
            warn("go(): do_connect() FAIL: [%d]%s", errno, strerror(errno));
            break;
        }
    }

    return connect;
}

// simple wrapper around do_write()
TcpRunStatus TcpRun::_go_write ()
{
    TcpRunStatus write = do_write();

    if (write != WRITE_OK)
        warn("go(): do_write() FAIL: [%d]%s", errno, strerror(errno));

    return write;
}

// preforms select() and recv() until there's no more data and retries on timeouts
TcpRunStatus TcpRun::_go_read ()
{
    TcpRunStatus result = NOOP;
    TcpRunStatus select = NOOP;
    TcpRunStatus recv   = NOOP;

    while (recv != RECV_DONE)
    {
        if (Global::premature_shutdown)
        {
            premature_shutdowns++;
            result = FORCED;
            break;
        }

        select = do_select();

        if (Global::premature_shutdown)
        {
            premature_shutdowns++;
            result = FORCED;
            break;
        }

        if (select == SELECT_OK)
        {
            recv = do_recv();

            if (recv == RECV_TIMEOUT)
            {
                timeouts++;

                if (max_timeouts >= 0 && timeouts > max_timeouts)
                {
                    warn("go(): do_recv() FAIL: too many timeouts");
                    break;
                }
            }
            else if (recv == RECV_RESET)
            {
                resets++;

                if (max_resets >= 0 && resets > max_resets)
                {
                    warn("go(): do_recv() RESET: [%d]%s", errno, strerror(errno));
                    break;
                }
            }
            else if (recv == RECV_FAIL)
            {
                warn("go(): do_recv() FAIL: [%d]%s", errno, strerror(errno));
                break;
            }
        }
        else if (select == SELECT_TIMEOUT)
        {
            timeouts++;

            if (max_timeouts >= 0 && timeouts > max_timeouts)
            {
                warn("go(): do_select() FAIL: too many timeouts");
                break;
            }
        }
        else if (select == SELECT_FAIL)
        {
            warn("go(): do_select() FAIL: [%d]%s", errno, strerror(errno));
            break;
        }
    }

    return result == NOOP ? (select == SELECT_OK ? recv : select) : result;

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

// the counterpart to a successful _go_connect()
TcpRunStatus TcpRun::_go_disconnect ()
{
    TcpRunStatus shutdown = NOOP;
    TcpRunStatus close = NOOP;

    shutdown = do_shutdown();

    if (shutdown == SHUTDOWN_FAIL)
        warn("go(): do_shutdown() FAIL: [%d]%s", errno, strerror(errno));

    close = do_close();

    if (close == CLOSE_FAIL)
        warn("go(): do_close() FAIL: [%d]%s", errno, strerror(errno));

    return shutdown == SHUTDOWN_OK ? close : shutdown;

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
    TcpRunStatus init = NOOP;
    TcpRunStatus connect = NOOP;
    TcpRunStatus write = NOOP;
    TcpRunStatus read = NOOP;
    TcpRunStatus disconnect = NOOP;

    timeouts            = 0;
    resets              = 0;
    bytes_written       = 0;
    bytes_read          = 0;
    premature_shutdowns = 0;

    //if ((init = _go_init()) == GETHOSTBYNAME_OK)
    if ((init = do_socket()) == SOCKET_OK)
    {
        if ((connect = _go_connect()) == CONNECT_OK)
        {
            if ((write = _go_write()) == WRITE_OK)
            {
                read = _go_read();
            }

            disconnect = _go_disconnect();
        }
    }

    // this is the return "statement"
    if (init == SOCKET_OK)
    {
        if (connect == CONNECT_OK)
        {
            if (disconnect == CLOSE_OK)
            {
                if (write == WRITE_OK)
                {
                    if (read == RECV_DONE)
                        return GREAT_SUCCESS;
                    else
                        return read;
                }
                else
                {
                    return write;
                }
            }
            else
            {
                if (write == WRITE_OK && read == RECV_DONE)
                    return disconnect;
                else
                    return MULTI_FAIL;
            }
        }
        else
        {
            return connect;
        }
    }
    else
    {
        return init;
    }
}

unsigned int TcpRun::get_connect_attempts ()
{
    return connect_attempts;
}

unsigned int TcpRun::get_timeouts ()
{
    return timeouts;
}

unsigned int TcpRun::get_resets ()
{
    return resets;
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
