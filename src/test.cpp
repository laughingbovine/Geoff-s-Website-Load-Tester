#include "test.h"

////////////////////////////////////////////////////////////////////////////////

LoadTest::Input::Input () :
    target(NULL),
    request(NULL),
    num_sessions(1),
    run_time(0)
{}

void LoadTest::Input::print ()
{
    printf("Number of Sessions: %d session(s)\n", num_sessions);
    printf("Run Time:           %d seconds\n", run_time);
}

////////////////////////////////////////////////////////////////////////////////

LoadTest::Result::Result () :
    num_sessions(0),
    num_failures(0),
    num_timeouts(0),
    num_bytes_written(0),
    num_bytes_read(0),
    num_premature_shutdowns(0),
    time(0.0f)
{}

void LoadTest::Result::print ()
{
    printf("Sessions:               %u\n",              num_sessions);
    printf("Failures:               %u\n",              num_failures);
    printf("Timeouts:               %u\n",              num_timeouts);
    printf("Bytes   Written:        %lu\n",             num_bytes_written);
    printf("        Read:           %lu\n",             num_bytes_read);
    printf("Time:                   %-10.2f seconds\n", time);

    if (num_premature_shutdowns > 0)
    printf("Premature Shutdowns:    %u\n",              num_premature_shutdowns);
}

////////////////////////////////////////////////////////////////////////////////

LoadTest::Test::Test () :
    last_run(NOOP)
{}

void LoadTest::Test::start ()
{
    int ret = pthread_create(&the_thread, NULL, LoadTest::Test::_start_thread, this);

    if (ret != 0)
        error("LoadTest::Test::start(): ERROR creating test thread: [%d:%d]%s", ret, errno, strerror(errno));
}

void LoadTest::Test::wait_and_finish ()
{
    int ret = pthread_join(the_thread, NULL);

    if (ret != 0 && ret != 3) // 3 == no such thread
        error("LoadTest::Test::wait_and_finish(): ERROR joining test thread: [%d]%s", ret, strerror(ret));
}

// this function is thread-friendly
// also static
void* LoadTest::Test::_start_thread (void* me)
{
    // and go!
    ((LoadTest::Test*)me)->_run();

    return NULL;
}

void LoadTest::Test::_run ()
{
    TcpRun run(input->target, input->request);
    Stopwatch clock;

    //printf("about to run:\n");
    //input->print();

    // start the clock
    clock.lap();

    do
    {
        last_run = run.go();

        if (last_run != GREAT_SUCCESS)
            result.num_failures++;

        result.num_sessions++;
        result.num_timeouts            += run.get_timeouts();
        result.num_bytes_written       += run.get_bytes_written();
        result.num_bytes_read          += run.get_bytes_read();
        result.num_premature_shutdowns += run.get_premature_shutdowns();

        result.time += clock.lap();

        if (Global::premature_shutdown)
            break;
    }
    while (
        (!(input->num_sessions <= 0 && input->run_time <= 0))
        &&
        (input->num_sessions <= 0 || result.num_sessions < input->num_sessions)  // number of sessions not yet reached
        &&
        (input->run_time <= 0 || (unsigned int)result.time < input->run_time)    // time isn't up
    );
}
