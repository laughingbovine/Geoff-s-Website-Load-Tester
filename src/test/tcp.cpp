#include "../tcp.h"

////////////////////////////////////////////////////////////////////////////////

int main (int argc, char** argv)
{
    TcpConnection t;

    t.connect("pulitzer2.lampdev.columbia.edu", 80);

    t.swrite("GET / HTTP/1.1\nHost: pulitzer2.lampdev.columbia.edu\nConnection: close\n\n");

    while (t.readline() > 0)
    {
        cout << t.last_read();
    }
}
