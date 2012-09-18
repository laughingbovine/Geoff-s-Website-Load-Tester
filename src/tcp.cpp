#include "tcp.h"

////////////////////////////////////////////////////////////////////////////////

int TcpConnection::_open_tcp_socket (const char *host_name, const int port_number)
{
    int new_socket_id;              // result from socket() call
    int connect_result;             // result from connect() call
    struct hostent *host;           // host object
    struct sockaddr_in server;      // server object

    new_socket_id = ::socket(AF_INET, SOCK_STREAM, 0); // create socket
    // AF_INET means that this socket is connecting to an internet address (AF_UNIX would be a unix socket instead)
    // SOCK_STREAM basically means "use TCP" (with SOCK_DGRAM for "use UDP")
    // 0 is for protocol, but should always be 0 which means it will choose the "most appropriate" protocol (TCP for STREAMs and UDP for DataGRAMs)
    if (new_socket_id < 0) // error check creation of socket
    {
        warn("TcpConnection::_open_tcp_socket(): ERROR opening socket");
        return -1;
    }

    // process host name input
    host = ::gethostbyname(host_name);
    if (host == NULL)
    {
        warn("TcpConnection::_open_tcp_socket(): ERROR, no such host");
        return -1;
    }

    // initialize the server address object
    bzero((char*)&server, sizeof(server));                                      // zero the server address object
    server.sin_family = AF_INET;                                                // (as above) server is on the internet
    bcopy((char*)host->h_addr, (char*)&server.sin_addr.s_addr, host->h_length); // copy server address string over to object
    server.sin_port = htons(port_number);                                       // convert port number

    // open connection
    connect_result = ::connect(new_socket_id, (struct sockaddr*)&server, sizeof(server));
    if (connect_result < 0)
    {
        warn("TcpConnection::_open_tcp_socket(): ERROR connecting");
        return -1;
    }

    return new_socket_id;
}

////////////////////////////////////////////////////////////////////////////////

TcpConnection::TcpConnection ()
{
    connected = false;

    // initialize timeout
    wait_timeout.tv_sec     = TCP_CONNECTION_TIMEOUT_SEC;
    wait_timeout.tv_usec    = TCP_CONNECTION_TIMEOUT_USEC;
}

int TcpConnection::connect (const char *host_name, const int port_number)
{
    if (connected)
        error("TcpConnection::connect() socket already connected");

    // store socket id
    socket_id = _open_tcp_socket(host_name, port_number);

    if (socket_id < 0)
    {
        warn("TcpConnection::connect() error getting socket");
        return -1;
    }

    // initialize socket "set"
    FD_ZERO(&socket_set);
    FD_SET(socket_id, &socket_set);

    connected = true;

    return 0;
}

void TcpConnection::shutdown_writes ()
{
    ::shutdown(socket_id, SHUT_WR);
}

void TcpConnection::shutdown_reads ()
{
    ::shutdown(socket_id, SHUT_RD);
}

void TcpConnection::disconnect ()
{
    ::shutdown(socket_id, SHUT_RDWR);
    ::close(socket_id);

    connected = false;
}

////////////////////////////////////////////////////////////////////////////////

int TcpConnection::write (const char *message, const int message_length)
{
    int num_bytes; // number of bytes written

    // write message to socket
    num_bytes = ::write(socket_id, message, message_length);

    if (num_bytes < 0) // error check write to socket
        error("TcpConnection::write(): ERROR writing to socket");
    else if (num_bytes < message_length)
        error("TcpConnection::write(): only wrote %d bytes out of %d", num_bytes, message_length);

    //::shutdown(socket_id, SHUT_WR);

    return num_bytes;
}

int TcpConnection::write (CharBuffer &b)
{
    return write(b.chars, b.size);
}

int TcpConnection::swrite (const char *message)
{
    return write(message, strlen(message));
}

int TcpConnection::swrite (string &message)
{
    return write(message.c_str(), message.length());
}

////////////////////////////////////////////////////////////////////////////////

bool TcpConnection::_is_read_ready ()
{
    int result; // select result

    result = ::select(FD_SETSIZE, &socket_set, NULL, NULL, &wait_timeout);

    if (result < 0)
    {
        warn("TcpConnection::_is_read_ready(): ERROR select()ing socket");
        timeout = true;
    }
    else if (result == 0)
    {
        warn("TcpConnection::_is_read_ready(): select() timeout");
        timeout = true; // timeout
    }
    else
    {
        timeout = false; // good to go
    }

    return !timeout;
}

bool TcpConnection::check_timeout ()
{
    return timeout;
}

////////////////////////////////////////////////////////////////////////////////

int TcpConnection::_get (char *buffer, const int buffer_size, const int mode)
{
    int num_bytes; // number of bytes read

    if (!_is_read_ready())
        return -1;

    // grab input from socket
    num_bytes = ::recv(socket_id, buffer, buffer_size, mode);

    if (num_bytes < 0) // error check read from socket
    {
        warn("TcpConnection::_get(): ERROR reading from socket");
        return -1;
    }

    return num_bytes;
}

int TcpConnection::_get_until (char *buffer, const int buffer_size, const char until, const int mode)
{
    int num_bytes;              // number of bytes read
    char* until_char_location;  // location of the "until" character
    int str_length;             // length of line to be read

    if (!_is_read_ready())
        return -1;

    // first, take a peek so we can find the "until" character
    // -1 so we can null-terminate the string (see below)
    num_bytes = ::recv(socket_id, buffer, buffer_size - 1, MSG_PEEK);

    if (num_bytes < 0)
    {
        warn("TcpConnection::_get_until(): ERROR reading from socket");
        return -1;
    }

    *(buffer+num_bytes) = '\0'; // null-terminate that string for strchr()

    until_char_location = strchr(buffer, until); // search for the "until" character

    if (until_char_location == NULL)
        return 0; // not found, buffer might not be big enough

    str_length = until_char_location - buffer;

    // +1 to eat the "until" character as well
    num_bytes = ::recv(socket_id, buffer, str_length + 1, mode);

    if (num_bytes < 0)
    {
        warn("TcpConnection::_get_until(): ERROR reading from socket");
        return -1;
    }

    *(buffer+num_bytes) = '\0'; // null-terminate that string, yo

    return num_bytes;
}

////////////////////////////////////////////////////////////////////////////////

/* read as much as possible
 * into the provided buffer
 */

int TcpConnection::read (char *buffer, const int buffer_size)
{
    return _get(buffer, buffer_size, 0);
}

int TcpConnection::read (CharBuffer &b)
{
    return read(b.chars, b.size);
}

int TcpConnection::read ()
{
    return read(buffer);
}

////////////////////////////////////////////////////////////////////////////////

/* read as much as possible up to and including a null character
 * or the maximum buffer size into the provided buffer
 */

int TcpConnection::sread (char *buffer, const int buffer_size)
{
    return _get_until(buffer, buffer_size, '\0', 0);
}

int TcpConnection::sread (CharBuffer &b)
{
    return sread(b.chars, b.size);
}

int TcpConnection::sread ()
{
    return sread(buffer);
}

////////////////////////////////////////////////////////////////////////////////

/* read as much as possible up to and including a newline character
 * or nothing! into the provided buffer
 */

int TcpConnection::readline (char *buffer, const int buffer_size)
{
    return _get_until(buffer, buffer_size, '\n', 0);
}

int TcpConnection::readline (CharBuffer &b)
{
    return readline(b.chars, b.size);
}

int TcpConnection::readline ()
{
    return readline(buffer);
}

////////////////////////////////////////////////////////////////////////////////

/* read all bytes there are to read into the internal buffer
 * optionally keep a count
 */

int TcpConnection::read_all (unsigned long* num_bytes)
{
    int read_bytes;

    while ((read_bytes = read()) > 0)
        if (num_bytes != NULL)
            *num_bytes += read_bytes;

    if (check_timeout())
        return -1;
    else
        return 0;
}

int TcpConnection::print_all (unsigned long* num_bytes)
{
    int read_bytes;

    while ((read_bytes = sread()) > 0)
    {
        if (num_bytes != NULL)
            *num_bytes += read_bytes;

        cout << last_read();
    }

    if (check_timeout())
        return -1;
    else
        return 0;
}

////////////////////////////////////////////////////////////////////////////////

const char* TcpConnection::last_read ()
{
    return buffer.chars;
}
