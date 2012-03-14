#include "test.h"

////////////////////////////////////////////////////////////////////////////////

void LoadTest::Test::start (LoadTest::Input &i)
{
    int ret;

    if (ret = pthread_create(&the_thread, NULL, LoadTest::Test::_run, (void*)&i))
        error("LoadTest::Test::start(): ERROR creating test thread, %d", ret);
}

LoadTest::Results& LoadTest::Test::wait_and_finish ()
{
    LoadTest::Results* tr;

    pthread_join(the_thread, (void**)&tr);

    return *tr;
}

// this function is thread-friendly
void* LoadTest::Test::_run (void* test_input)
{
    LoadTest::Input* ti = (LoadTest::Input*)test_input;
    //LoadTest::Results* tr = malloc(sizeof(LoadTest::Results));
    LoadTest::Results* tr = new LoadTest::Results();
    //tcp_connection* conn;
    TcpConnection conn;
    //struct timeval* clock;
    Stopwatch clock;
    int i;

    // initialize variables
    tr->num_bytes_read = 0;
    tr->num_bytes_written = 0;
    tr->num_timeouts = 0;
    tr->num_sessions = 0;
    tr->time = 0.0;

    // start the clock
    clock.lap();

    while (1)
    {
        // establish connection
        //connect_tcp(conn, ti->host_name, ti->port_number);
        conn.connect(ti->host_name, ti->port_number);

        //cout << "=====SENDING=====" << endl << *ti->input << endl;

        // send out the requests
        //swrite_tcp(conn, ti->input);
        tr->num_bytes_written += conn.write(*ti->input);
        conn.shutdown_writes();

        //cout << "=====RECIEVING=====" << endl;

        // read the responses and check for a timeout
        //if (conn.print_all(&tr->num_bytes_read) == -1)
        if (conn.read_all(&tr->num_bytes_read) == -1)
            tr->num_timeouts++;

        // cleanup
        //disconnect_tcp(conn);
        conn.disconnect();

        // update stats
        tr->num_sessions++;
        //tr->time += stopwatch(&clock);
        tr->time += clock.lap();

        // check if we're done looping
        if (
            (ti->run_time_seconds > 0 && (unsigned int)tr->time >= ti->run_time_seconds)    // time is up
            ||
            (ti->num_sessions > 0 && tr->num_sessions >= ti->num_sessions)                  // number of sessions reached
            ||
            (ti->run_time_seconds <= 0 && ti->num_sessions <= 0)                            // no repeat method specified (one run only)
            )
            break;
    }

    //pthread_exit((void*)&tr);
    return (void*)tr;
}
