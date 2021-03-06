#include <stdio.h>
#include <stdlib.h>

#include "../tcp.h"

////////////////////////////////////////////////////////////////////////////////

int main (int argc, char** argv)
{
    CharBuffer request("GET / HTTP/1.0\r\nConnection: close\r\n\r\n");
    sockaddr_in target;
    TcpRun t(0, &target, &request);

    if (!resolve_host_name(&target, "www.columbia.edu", 80))
    {
        printf("whoops...");
        exit(1);
    }

    TcpRunStatus trs = t.go();

    if (trs == GREAT_SUCCESS)
    {
        printf("success!\n");
    }
    else
    {
        printf("failure\n");
    }

    printf("%s\n", TcpRunStatusStrings[trs]);
    t.print();
}
