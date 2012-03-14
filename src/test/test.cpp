#include "../test.h"

////////////////////////////////////////////////////////////////////////////////

int main (int argc, char** argv)
{
    //cout << __FILE__ << " " << __LINE__ << endl;

    LoadTest::Test t;
    LoadTest::Input ti;

    ti.host_name = "pulitzer2.lampdev.columbia.edu";
    ti.port_number = 80;
    ti.input = new CharBuffer("GET / HTTP/1.1\nHost: pulitzer2.lampdev.columbia.edu\nConnection: close\n\n");
    ti.num_sessions = 10;
    ti.run_time_seconds = 0;

    t.start(ti);
    LoadTest::Results tr = t.wait_and_finish();

    //LoadTest::Results tr = *((LoadTest::Results*)LoadTest::run((void*)&ti));

    cout << "time: " << tr.time << endl;
    cout << "bytes: " << tr.num_bytes_read << endl;
    cout << "timeouts: " << tr.num_timeouts << endl;
    cout << "sessions: " << tr.num_sessions << endl;
}
