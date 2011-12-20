#include "tcp.h"

tcp_connection* _set_tcp_socket (tcp_connection* this, int new_socket_id)
{
    // initialize socket
    this->socket_id = new_socket_id;

    // initialize socket set
    FD_ZERO(&this->socket_set);
    FD_SET(this->socket_id, &this->socket_set);

    return this;
}

int _open_tcp_socket (const char *host_name, const int port_number)
{
    int new_socket_id;              // result from socket() call
    int connect_result;             // result from connect() call
    struct hostent *host;           // host object
    struct sockaddr_in server;      // server object

    new_socket_id = socket(AF_INET, SOCK_STREAM, 0); // create socket
    // AF_INET means that this socket is connecting to an internet address (AF_UNIX would be a unix socket instead)
    // SOCK_STREAM basically means "use TCP" (with SOCK_DGRAM for "use UDP")
    // 0 is for protocol, but should always be 0 which means it will choose the "most appropriate" protocol (TCP for STREAMs and UDP for DataGRAMs)
    if (new_socket_id < 0) // error check creation of socket
        error("_open_tcp_socket(): ERROR opening socket");

    // process host name input
    host = gethostbyname(host_name);
    if (host == NULL)
        error("_open_tcp_socket(): ERROR, no such host");

    // initialize the server address object
    bzero((char*)&server, sizeof(server));                                      // zero the server address object
    server.sin_family = AF_INET;                                                // (as above) server is on the internet
    bcopy((char*)host->h_addr, (char*)&server.sin_addr.s_addr, host->h_length); // copy server address string over to object
    server.sin_port = htons(port_number);                                       // convert port number

    // open connection
    connect_result = connect(new_socket_id, (struct sockaddr*)&server, sizeof(server));
    if (connect_result < 0)
        error("_open_tcp_socket(): ERROR connecting");

    return new_socket_id;
}

tcp_connection* new_tcp_connection_object ()
{
    tcp_connection* this;
    this = malloc(sizeof(tcp_connection));

    // initalize buffer
    this->buffer = malloc(TCP_CONNECTION_BUFFER_SIZE * sizeof(char));
    this->buffer_size = TCP_CONNECTION_BUFFER_SIZE;

    // initialize timeout
    this->wait_timeout_sec  = TCP_CONNECTION_TIMEOUT_SEC;
    this->wait_timeout_usec = TCP_CONNECTION_TIMEOUT_USEC;

    return this;
}

void connect_tcp (tcp_connection* this, const char *host_name, const int port_number)
{
    _set_tcp_socket(this, _open_tcp_socket(host_name, port_number));
}

void disconnect_tcp (tcp_connection* this)
{
    close(this->socket_id);
}

void free_tcp (tcp_connection* this)
{
    free(this->buffer);
    free(this);
}

////////////////////////////////////////////////////////////////////////////////
// Input/Output ////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

int swrite_tcp (tcp_connection* this, const char* message)
{
    int num_bytes; // number of bytes written

    // write message to socket
    num_bytes = write(this->socket_id, message, strlen(message));

    if (num_bytes < 0) // error check write to socket
        error("swrite_tcp(): ERROR writing to socket");

    return num_bytes;
}

int sread_tcp (tcp_connection* this)
{
    int num_bytes; // number of bytes read

    // grab input from socket (- 1 to make room for the null char)
    //printf("before read...\n");
    //num_bytes = read(socket_id, read_buffer, read_buffer_size - 1);
    num_bytes = recv(this->socket_id, this->buffer, this->buffer_size - 1, 0);
    //printf("after read...\n");

    if (num_bytes < 0) // error check read from socket
        error("sread_tcp(): ERROR reading from socket");

    *(this->buffer+num_bytes) = '\0'; // null-terminate that string, yo

    return num_bytes;
}

int sreadline_tcp (tcp_connection* this)
{
    int num_bytes;          // number of bytes read
    char* newline_location; // location of '\n' character
    int line_length;        // length of line to be read

    // first, take a peek so we can find that '\n'
    // (- 1 to make room for the null char
    // odd to do this this time around, but consider what happens if the '\n' is the last char)
    num_bytes = recv(this->socket_id, this->buffer, this->buffer_size - 1, MSG_PEEK);

    if (num_bytes < 0)
        error("sreadline_tcp(): ERROR reading from socket");

    *(this->buffer+num_bytes) = '\0'; // null-terminate that string, for strchr()

    newline_location = strchr(this->buffer, '\n'); // search for '\n'

    if (newline_location == NULL)
        return -1; // not found

    line_length = (newline_location - this->buffer) / sizeof(char);

    // this time, actually do a "read" (instead of a "peek")
    // (+ 1 to eat the newline as well)
    // math says line_length + 1 will not be greater than this->buffer_size
    num_bytes = recv(this->socket_id, this->buffer, line_length + 1, 0);

    if (num_bytes < 0)
        error("sreadline_tcp(): ERROR reading from socket");

    *(this->buffer+num_bytes) = '\0'; // null-terminate that string, yo

    return num_bytes;
}

// readline and allocate a string result
//int sreadline_alloc_tcp (...)

int wait_tcpread (tcp_connection* this)
{
    int result; // select result

    // (re-)initialize timeout
    this->wait_timeout.tv_sec   = this->wait_timeout_sec;
    this->wait_timeout.tv_usec  = this->wait_timeout_usec;

    result = select(FD_SETSIZE, &this->socket_set, NULL, NULL, &this->wait_timeout);

    if (result < 0)
        error("wait_tcpread(): ERROR select()ing socket");

    return result;
}

int readall_tcp (tcp_connection* this, unsigned long* num_bytes)
{
    int read_result, wait_result;
    
    while (1)
    {
        wait_result = wait_tcpread(this);

        if (wait_result == 0)
            break; // timeout

        read_result = sread_tcp(this);

        if (read_result == 0)
            break; // (normal) closed connection

        if (num_bytes != NULL)
            *num_bytes += read_result;

        //printf("%s", read_buffer);
        //printf("\n\n --- take a breath --- \n\n");
    }

    if (wait_result == 0)
        return -1;
    else
        return 0;
}
