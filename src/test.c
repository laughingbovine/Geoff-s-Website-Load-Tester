#include "test.h"

void* run_test (void* _test_input)
{
    test_input* ti = (test_input*)_test_input;
    test_results* tr = malloc(sizeof(test_results));
    tcp_connection* conn;
    struct timeval* clock;
    int i;

    // initialize variables
    tr->num_bytes = 0;
    tr->num_timeouts = 0;
    tr->num_sessions = 0;
    tr->time = 0.0;
    clock = NULL;
    conn = new_tcp_connection_object();

    // start the clock
    tr->time = stopwatch(&clock);

    while (1)
    {
        // establish connection
        connect_tcp(conn, ti->host_name, ti->port_number);

        // send out the requests
        swrite_tcp(conn, ti->input);

        // read the responses and check for a timeout
        if (readall_tcp(conn, &tr->num_bytes))
            tr->num_timeouts++;

        // cleanup
        disconnect_tcp(conn);

        // update stats
        tr->num_sessions++;
        tr->time += stopwatch(&clock);

        if (
            (ti->run_time_seconds > 0 && (unsigned int)tr->time >= ti->run_time_seconds)    // time is up
            ||
            (ti->num_sessions > 0 && tr->num_sessions >= ti->num_sessions)                  // number of sessions reached
            ||
            (ti->run_time_seconds <= 0 && ti->num_sessions <= 0)                            // no repeat method specified (one run only)
            )
            break;
    }

    // cleanup
    free_tcp(conn);
    free(clock);

    pthread_exit((void*)tr);
}

void free_test_input (test_input* obj)
{
    if (obj->input != NULL)
        free(obj->input);
    free(obj);
}

void free_test_results (test_results* obj)
{
    free(obj);
}
