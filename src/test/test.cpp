#include "../test.h"

////////////////////////////////////////////////////////////////////////////////

int main (int argc, char** argv)
{
    LoadTest::Input input;
    sockaddr_in target;
    CharBuffer request("GET / HTTP/1.0\r\nConnection: close\r\n\r\n");

    resolve_host_name(&target, "www.columbia.edu", 80);

    input.target = &target;
    input.request = &request;
    input.num_sessions = 1;
    input.run_time = 0;

    LoadTest::Test test;
    test.input = &input;

    test.start();
    test.wait_and_finish();

    test.result.print();
}
