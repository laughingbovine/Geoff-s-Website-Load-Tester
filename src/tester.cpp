#include "tester.h"

void start_premature_shutdown (int param)
{
    Global::premature_shutdown = true;
    fprintf(stderr, "\nPremature shutdown initiated.  Please wait...\n");
}

////////////////////////////////////////////////////////////////////////////////

LoadTest::Tester::Tester (const char* host_name, int port_number, unsigned int num_sessions, unsigned int run_time) :
    host_name(host_name),
    port_number(port_number),
    num_sessions(num_sessions),
    run_time(run_time),
    num_tests_per_ramp(1),
    num_ramps(1),
    ramp_wait(0),
    total_sessions(0),
    total_failures(0),
    total_timeouts(0),
    total_resets(0),
    total_bytes_written(0),
    total_bytes_read(0),
    total_premature_shutdowns(0),
    total_time(0.0f),
    last_num_tests_running(0),
    last_num_tests_finished(0),
    num_tests_running(0),
    num_tests_finished(0)
{
    if (!resolve_host_name(&target, host_name, port_number))
        error("Fatal error.  Perhaps you're not online?"); // not sure how to do this well
}

LoadTest::Tester::~Tester ()
{
    for (int i = 0; i < inputs.size(); i++)
        delete inputs[i];
}

////////////////////////////////////////////////////////////////////////////////

void LoadTest::Tester::add_input (CharBuffer& request)
{
    LoadTest::Input* i = new LoadTest::Input();

    i->target       = &target;
    i->request      = &request;
    i->num_sessions = num_sessions;
    i->run_time     = run_time;

    inputs.push_back(i);
}

void LoadTest::Tester::set_num_tests_per_ramp (unsigned int x)
{
    num_tests_per_ramp = x;

    num_tests = num_ramps * num_tests_per_ramp;
}

void LoadTest::Tester::set_num_ramps (unsigned int x)
{
    num_ramps = x;

    num_tests = num_ramps * num_tests_per_ramp;
}

void LoadTest::Tester::set_ramp_wait (unsigned int x)
{
    ramp_wait = x;
}

////////////////////////////////////////////////////////////////////////////////

void LoadTest::Tester::print_initial_summary ()
{
    printf("Summary:\n");
    printf("===================\n");

    //printf("maximum number of threads: %ld or %d\n", sysconf(_SC_THREADS), _POSIX_THREAD_THREADS_MAX);

    if (num_ramps > 1)
        printf("running with %d x %d (= %d) users\n", num_tests_per_ramp, num_ramps, num_tests_per_ramp * num_ramps);
    else if (num_tests_per_ramp > 1)
        printf("running with %d users\n", num_tests_per_ramp);
    else
        printf("running with 1 user\n");

    if (num_ramps > 1)
        printf("waiting %d second(s) between ramps\n", ramp_wait);

    if (num_sessions > 0 && run_time > 0)
        printf("each doing %d session(s) or running for %d second(s) (whichever comes first)\n", num_sessions, run_time);
    else if (num_sessions > 0)
        printf("each doing %d session(s)\n", num_sessions);
    else if (run_time > 0)
        printf("each running for at least %d second(s)\n", run_time);
    else
        printf("each doing 1 session\n", num_sessions);

    printf("targeting %s port %d\n", host_name, port_number);

    printf("...\n");
}

void LoadTest::Tester::start_signal_handlers ()
{
    signal(SIGINT, start_premature_shutdown);
    signal(SIGTERM, start_premature_shutdown);
}


void LoadTest::Tester::init_tests ()
{
    // create the test objects
    // assign a different input to each test by cycling through all of them

    tests.resize(num_tests);

    for (int i = 0; i < num_tests; i++)
    {
        tests[i] = new LoadTest::Test();

        tests[i]->input = inputs[i%inputs.size()];
    }
}

void LoadTest::Tester::run_tests ()
{
    for (int j = 0; j < num_ramps; j++)
    {
        //if (num_ramps > 1)
        //    printf("ramp\n");
        //    //printf("number of threads increasing to %d...\n", num_tests_per_ramp * (j + 1));

        for (int i = 0; i < num_tests_per_ramp; i++)
        {
            tests[(j*num_tests_per_ramp)+i]->start();

            if (Global::premature_shutdown) break;
        }

        if (Global::premature_shutdown) break;

        if (j < (num_ramps - 1) && ramp_wait > 0)
            sleep(ramp_wait);

        if (Global::premature_shutdown) break;
    }
}

//void LoadTest::Tester::finish_tests ()
//{
//    // now join them all back and do some metrics
//    for (int i = 0; i < num_tests; i++)
//    {
//        tests[i]->wait_and_finish();
//
//        total_sessions              += tests[i]->result.num_sessions;
//        total_failures              += tests[i]->result.num_failures;
//        total_timeouts              += tests[i]->result.num_timeouts;
//        total_resets                += tests[i]->result.num_resets;
//        total_bytes_written         += tests[i]->result.num_bytes_written;
//        total_bytes_read            += tests[i]->result.num_bytes_read;
//        total_premature_shutdowns   += tests[i]->result.num_premature_shutdowns;
//        total_time                  += tests[i]->result.time;
//
//        delete tests[i];
//    }
//}

void LoadTest::Tester::print_final_summary ()
{
    printf("Totals:\n");
    printf("===================\n");

    if (total_failures > 0)
    printf("Sessions:               %-10u (%u failed)\n",                       total_sessions, total_failures);
    else
    printf("Sessions:               %-10u\n",                                   total_sessions, total_failures);

    printf("                        %-10.2f sessions per second\n",             total_sessions / (total_time / num_tests));
    printf("                        %-10.2f sessions per thread\n",             (double)total_sessions / num_tests);
    printf("                        %-10.2f sessions per thread per second\n",  total_sessions / total_time);
    printf("\n");

    printf("Total Time:             %-10.2f seconds\n",                         total_time);
    printf("                        %-10.2f average seconds per thread\n",      total_time / num_tests);
    printf("\n");

    printf("Data (down):            %-10.2f MiB\n",                             (double)total_bytes_read / (1024 * 1024));
    printf("                        %-10.2f average KiBps\n",                   (total_bytes_read / 1024) / (total_time / num_tests));
    printf("                        %-10.2f average KiBps per thread\n",        (total_bytes_read / 1024) / total_time);
    printf("\n");

    printf("Data (up):              %-10.2f MiB\n",                             (double)total_bytes_written / (1024 * 1024));
    printf("                        %-10.2f average KiBps\n",                   (total_bytes_written / 1024) / (total_time / num_tests));
    printf("                        %-10.2f average KiBps per thread\n",        (total_bytes_written / 1024) / total_time);
    printf("\n");

    printf("Number of Timeouts:     %-10u\n",                                   total_timeouts);
    printf("Number of Resets:       %-10u\n",                                   total_resets);

    if (total_premature_shutdowns > 0)
    printf("Premature Shutdowns:    %-10u\n",                                   total_premature_shutdowns);
}

// test finisher thread stuff
////////////////////////////////////////////////////////////////////////////////

void LoadTest::Tester::finish_test (int i)
{
    tests[i]->wait_and_finish();

    total_sessions              += tests[i]->result.num_sessions;
    total_failures              += tests[i]->result.num_failures;
    total_timeouts              += tests[i]->result.num_timeouts;
    total_resets                += tests[i]->result.num_resets;
    total_bytes_written         += tests[i]->result.num_bytes_written;
    total_bytes_read            += tests[i]->result.num_bytes_read;
    total_premature_shutdowns   += tests[i]->result.num_premature_shutdowns;
    total_time                  += tests[i]->result.time;

    delete tests[i];

    tests[i] = NULL;
}

void LoadTest::Tester::cleanup_bad_test (int i)
{
    delete tests[i];

    tests[i] = NULL;
}

void LoadTest::Tester::finish_tests_start ()
{
    int ret = pthread_create(&finish_tests_thread, NULL, LoadTest::Tester::finish_tests_init, this);

    if (ret != 0)
    {
        error("LoadTest::Tester::finish_tests_start(): ERROR creating finisher thread: [%d:%d]%s", ret, errno, strerror(errno));
    }
}

void* LoadTest::Tester::finish_tests_init (void* me)
{
    // and go!
    ((LoadTest::Tester*)me)->finish_tests_loop();

    return NULL;
}

void LoadTest::Tester::finish_tests_loop ()
{
    TestStatus stat;

    printf("%5d waiting + %5d running + %5d finished = %5d total\n", num_tests, 0, 0, num_tests);

    while (true)
    {
        last_num_tests_running = num_tests_running;
        last_num_tests_finished = num_tests_finished;

        num_tests_running = 0;

        for (int i = 0; i < num_tests; i++)
        {
            if (tests[i] != NULL)
            {
                stat = tests[i]->get_status();

                if (stat == DONE_RUNNING)
                {
                    finish_test(i);
                    num_tests_finished++;
                }
                else if (stat == CREATE_THREAD_FAIL || (stat == JUST_CREATED && Global::premature_shutdown))
                {
                    cleanup_bad_test(i);
                    num_tests_finished++;
                }
                else if (stat == RUNNING)
                {
                    num_tests_running++;
                }
            }
        }

        if (num_tests_running != last_num_tests_running || num_tests_finished != last_num_tests_finished)
            printf("%5d waiting + %5d running + %5d finished = %5d total\n",
                num_tests - (num_tests_finished + num_tests_running), num_tests_running, num_tests_finished, num_tests);

        if (num_tests_finished >= num_tests)
            break;

        sleep(TESTER_FINISH_TEST_THREAD_SWEEPTIME);
    }
}

void LoadTest::Tester::finish_tests_finish ()
{
    int ret = pthread_join(finish_tests_thread, NULL);

    if (ret != 0)
    {
        warn("LoadTest::Tester::finish_tests_finish(): ERROR joining finisher thread: [%d:%d]%s", ret, errno, strerror(errno));
    }
}
