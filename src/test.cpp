#include "test.h"

const char* TestStatusStrings [TEST_TESTSTATUS_SIZE] = {
    "JUST_CREATED",
    "STARTING",
    "CREATE_THREAD_FAIL",
    "RUNNING",
    "DONE_RUNNING",
    "JOINING",
    "JOIN_THREAD_FAIL",
    "JOINED"
};

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
    num_connect_attempts(0),
    num_connect_successes(0),
    num_timeouts(0),
    num_bytes_written(0),
    num_bytes_read(0),
    num_premature_shutdowns(0),
    time(0.0f)
{
    for (int i = 0; i < TCP_TCPRUNSTATUS_SIZE; i++) {
        stats[i] = 0;
    }
}

void LoadTest::Result::print ()
{
    printf("Sessions:               %u\n",              num_sessions);
    printf("Failures:               %u\n",              num_failures);
    printf("Connect Attempts:       %u\n",              num_connect_attempts);
    printf("Connect Successes:      %u\n",              num_connect_successes);
    printf("Timeouts:               %u\n",              num_timeouts);
    printf("Bytes   Written:        %lu\n",             num_bytes_written);
    printf("        Read:           %lu\n",             num_bytes_read);
    printf("Time:                   %-10.2f seconds\n", time);

    if (num_premature_shutdowns > 0)
    printf("Premature Shutdowns:    %u\n",              num_premature_shutdowns);
}

////////////////////////////////////////////////////////////////////////////////

LoadTest::Test::Test (unsigned int id) :
    id(id),
    run(NULL),
    last_run(NOOP),
    status(JUST_CREATED)
{
    //printf("[%5u] test creation\n", id);
    pthread_mutex_init(&the_mutex, NULL);
}

LoadTest::Test::~Test ()
{
    //printf("[%5u] test destruction (status %s)\n", id, TestStatusStrings[get_status()]);
    pthread_mutex_destroy(&the_mutex);
}

TestStatus LoadTest::Test::get_status ()
{
    TestStatus ret;

    pthread_mutex_lock(&the_mutex);
    ret = status;
    pthread_mutex_unlock(&the_mutex);   

    return ret;
}

void LoadTest::Test::set_status (TestStatus s)
{
    //printf("[%5u] LoadTest::Test::set_status(): status changing from %s to %s\n", id, TestStatusStrings[get_status()], TestStatusStrings[s]);
    pthread_mutex_lock(&the_mutex);
    //printf("[%5u] LoadTest::Test::set_status(): status changing 2\n", id);
    status = s;
    //printf("[%5u] LoadTest::Test::set_status(): status changing 3\n", id);
    pthread_mutex_unlock(&the_mutex);   
    //printf("[%5u] LoadTest::Test::set_status(): status done changing\n", id);
}

void LoadTest::Test::start ()
{
    set_status(STARTING);

    int ret = pthread_create(&the_thread, NULL, LoadTest::Test::_start_thread, this);

    if (ret != 0)
    {
        set_status(CREATE_THREAD_FAIL);
        printf("[%5u] LoadTest::Test::start(): ERROR creating test thread: [%d:%d]%s\n", id, ret, errno, strerror(errno));
    }
}

void LoadTest::Test::wait_and_finish ()
{
    set_status(JOINING);

    //printf("[%5u] LoadTest::Test::wait_and_finish(): joining...\n", id);

    int ret = pthread_join(the_thread, NULL);

    if (ret != 0 && ret != 3) // 3 == no such thread
    {
        set_status(JOIN_THREAD_FAIL);
        printf("[%5u] LoadTest::Test::wait_and_finish(): ERROR joining test thread: [%d]%s\n", id, ret, strerror(ret));
    }
    else
    {
        set_status(JOINED);
        //printf("[%5u] LoadTest::Test::wait_and_finish(): joined!\n", id);
    }
}

void LoadTest::Test::print ()
{
    printf("Test Input:\n");
    input->print();

    printf("Test Status: %s\n", TestStatusStrings[get_status()]);

    printf("Test Result:\n");
    result.print();
}

// this function is thread-friendly
// also static
void* LoadTest::Test::_start_thread (void* me)
{
    // first set signal mask so that this thread handles NO signals
    sigset_t signal_set;
    sigemptyset(&signal_set);
    pthread_sigmask(SIG_SETMASK, &signal_set, NULL);

    // then print some sort of thread id
    //pthread_t pt = pthread_self();
    //printf("LoadTest::Test::_start_thread(): thread started ");
    //print_pthread_t(pt);
    //printf("\n");

    // and go!
    ((LoadTest::Test*)me)->_run();

    return NULL;
}

void LoadTest::Test::_run ()
{
    set_status(RUNNING);

    run = new TcpRun(id, input->target, input->request);
    last_run = NOOP;
    consecutive_failures = 0;

    // start the clock
    clock.lap();

    do
    {
        last_run = run->go();

        result.stats[last_run]++;

        result.num_sessions++;
        result.num_connect_attempts     += run->get_connect_attempts();
        result.num_connect_successes    += run->get_connect_successes();
        result.num_timeouts             += run->get_timeouts();
        result.num_bytes_written        += run->get_bytes_written();
        result.num_bytes_read           += run->get_bytes_read();
        result.num_premature_shutdowns  += run->get_premature_shutdowns();

        result.time += clock.lap();

        if (last_run != GREAT_SUCCESS) {
            result.num_failures++;

            if (!Global::premature_shutdown) {
                consecutive_failures++;
                //printf("[%5u] LoadTest::Test::_run(): taking a break x %u\n", id, consecutive_failures);
                sleep(consecutive_failures * TEST_FAILURE_SLEEPTIME);
            }
        } else {
            consecutive_failures = 0;
        }

        if (Global::premature_shutdown) {
            //printf("[%5u] LoadTest::Test::_run(): premature_shutdown\n", id);
            break;
        }
    }
    while (
        (!(input->num_sessions <= 0 && input->run_time <= 0))
        &&
        (input->num_sessions <= 0 || result.num_sessions < input->num_sessions)  // number of sessions not yet reached
        &&
        (input->run_time <= 0 || (unsigned int)result.time < input->run_time)    // time isn't up
    );

    delete run;

    set_status(DONE_RUNNING);
}
