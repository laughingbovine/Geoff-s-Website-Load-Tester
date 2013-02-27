#include "tester.h"

void start_premature_shutdown (int sig)
{
    Global::premature_shutdown = true;
    fprintf(stderr, "\nPremature shutdown initiated.  Please wait...\n");

    //pthread_t pt = pthread_self();
    //printf("start_premature_shutdown(): called from thread ");
    //print_pthread_t(pt);
    //printf("\n");
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
    total_connect_attempts(0),
    total_connect_successes(0),
    total_timeouts(0),
    total_bytes_written(0),
    total_bytes_read(0),
    total_premature_shutdowns(0),
    total_time(0.0f),
    num_tests_running(0),
    num_tests_finished(0),
    last_num_tests_running(0),
    last_num_tests_finished(0)
{
    if (!resolve_host_name(&target, host_name, port_number)) {
        printf("Fatal error.  Perhaps you're not online?\n"); // not sure how to do this well
        exit(1);
    }

    for (int i = 0; i < TCP_TCPRUNSTATUS_SIZE; i++) {
        total_stats[i] = 0;
    }
}

LoadTest::Tester::~Tester ()
{
    for (int i = 0; i < inputs.size(); i++)
        delete inputs[i];
}

////////////////////////////////////////////////////////////////////////////////

void LoadTest::Tester::add_input (CharBuffer* request)
{
    LoadTest::Input* i = new LoadTest::Input();

    i->target       = &target;
    i->request      = request;
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
    struct sigaction action_premature_shutdown;
    //struct sigaction action_premature_shutdown, action_print_stack_trace_and_exit;

    action_premature_shutdown.sa_handler = start_premature_shutdown;
    sigemptyset(&action_premature_shutdown.sa_mask);
    action_premature_shutdown.sa_flags = 0;

    //action_print_stack_trace_and_exit.sa_handler = print_stack_trace_and_exit;

    sigaction(SIGINT, &action_premature_shutdown, NULL);
    sigaction(SIGTERM, &action_premature_shutdown, NULL);
    //sigaction(SIGSEGV, &action_print_stack_trace_and_exit, NULL);

    //signal(SIGINT, start_premature_shutdown);
    //signal(SIGTERM, start_premature_shutdown);
    //signal(SIGSEGV, print_stack_trace_and_exit);
}

void LoadTest::Tester::init_tests ()
{
    // create the test objects
    // assign a different input to each test by cycling through all of them

    tests.resize(num_tests);

    for (int i = 0; i < num_tests; i++)
    {
        tests[i] = new LoadTest::Test(i+2);

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

            if (Global::premature_shutdown) {
                printf("[     ] LoadTest::Tester::run_tests(): premature_shutdown Mid test ramp\n");
                break;
            }
        }

        if (Global::premature_shutdown) {
            printf("[     ] LoadTest::Tester::run_tests(): premature_shutdown After test ramp\n");
            break;
        }

        if (j < (num_ramps - 1) && ramp_wait > 0)
            sleep(ramp_wait);

        if (Global::premature_shutdown) {
            printf("[     ] LoadTest::Tester::run_tests(): premature_shutdown after Wake up\n");
            break;
        }
    }
}

void LoadTest::Tester::print_final_summary ()
{
    printf("Totals:\n");
    printf("===================\n");

    printf("Sessions:               %-10u successful\n",                        total_sessions - total_failures);
    if (total_failures > 0)
    printf("                        %-10u failed\n",                            total_failures);

    printf("                        %-10.2f sessions per second\n",             total_sessions / (total_time / num_tests));
    printf("                        %-10.2f sessions per thread\n",             (double)total_sessions / num_tests);
    printf("                        %-10.2f sessions per thread per second\n",  total_sessions / total_time);
    printf("Connect Percentage:     %-10.2f %%\n",                              ((double)total_connect_successes / total_connect_attempts) * 100);
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
    printf("                        %-10.2f average per thread\n",              (double)total_timeouts / num_tests);

    if (total_premature_shutdowns > 0)
    printf("Premature Shutdowns:    %-10u\n",                                   total_premature_shutdowns);

    for (int i = 0; i < TCP_TCPRUNSTATUS_SIZE; i++)
        if (total_stats[i] > 0)
            printf("%-24s%d\n", TcpRunStatusStrings[i], total_stats[i]);
}

// test finisher thread stuff
////////////////////////////////////////////////////////////////////////////////

void LoadTest::Tester::finish_test (int i)
{
    //printf("finishing test %d\n", i+2);

    tests[i]->wait_and_finish();

    for (int j = 0; j < TCP_TCPRUNSTATUS_SIZE; j++) {
        total_stats[j] += tests[i]->result.stats[j];
    }

    total_sessions              += tests[i]->result.num_sessions;
    total_failures              += tests[i]->result.num_failures;
    total_connect_attempts      += tests[i]->result.num_connect_attempts;
    total_connect_successes     += tests[i]->result.num_connect_successes;
    total_timeouts              += tests[i]->result.num_timeouts;
    total_bytes_written         += tests[i]->result.num_bytes_written;
    total_bytes_read            += tests[i]->result.num_bytes_read;
    total_premature_shutdowns   += tests[i]->result.num_premature_shutdowns;
    total_time                  += tests[i]->result.time;

    delete tests[i];

    tests[i] = NULL;
}

void LoadTest::Tester::cleanup_bad_test (int i)
{
    //printf("cleaning up bad test %d\n", i+2);

    delete tests[i];

    tests[i] = NULL;
}

void LoadTest::Tester::finish_tests_start ()
{
    int ret = pthread_create(&finish_tests_thread, NULL, LoadTest::Tester::finish_tests_init, this);

    //pthread_t pt = pthread_self();
    //printf("LoadTest::Tester::finish_tests_start(): main thread ");
    //print_pthread_t(pt);
    //printf(" creating test finisher thread ");
    //print_pthread_t(finish_tests_thread);
    //printf("\n");

    if (ret != 0) {
        printf("LoadTest::Tester::finish_tests_start(): ERROR creating finisher thread: [%d:%d]%s\n", ret, errno, strerror(errno));
        exit(1);
    }
}

void* LoadTest::Tester::finish_tests_init (void* me)
{
    // first set signal mask so that this thread handles NO signals
    sigset_t signal_set;
    sigemptyset(&signal_set);
    pthread_sigmask(SIG_SETMASK, &signal_set, NULL);

    // then print some sort of thread id
    //pthread_t pt = pthread_self();
    //printf("LoadTest::Tester::finish_tests_init(): thread started ");
    //print_pthread_t(pt);
    //printf("\n");

    // and go!
    ((LoadTest::Tester*)me)->finish_tests_loop();

    return NULL;
}

void LoadTest::Tester::finish_tests_loop ()
{
    TestStatus stat;

    while (true) {
        last_num_tests_running = num_tests_running;
        last_num_tests_finished = num_tests_finished;

        num_tests_running = 0;
        num_tests_finished = 0;

        for (int i = 0; i < num_tests; i++) {
            if (tests[i] != NULL) {
                stat = tests[i]->get_status();

                if (stat == DONE_RUNNING) {
                    finish_test(i);
                    num_tests_finished++;
                } else if (stat == CREATE_THREAD_FAIL || stat == JOIN_THREAD_FAIL) {
                    printf("thread error\n");
                    cleanup_bad_test(i);
                    num_tests_finished++;
                } else if (Global::premature_shutdown && stat == JUST_CREATED) {
                    cleanup_bad_test(i);
                    num_tests_finished++;
                } else if (stat == RUNNING) {
                    num_tests_running++;
                }
            } else {
                num_tests_finished++;
            }
        }

        if (num_tests_running != last_num_tests_running || num_tests_finished != last_num_tests_finished) {
            printf("%5d waiting + %5d running + %5d finished = %5d total\n",
                num_tests - (num_tests_finished + num_tests_running), num_tests_running, num_tests_finished, num_tests);
        }

        if (num_tests_finished >= num_tests) break;

        //printf("LoadTest::Tester::finish_tests_loop(): sleeping...\n");

        fflush(stdout);

        sleep(TESTER_FINISH_TEST_THREAD_SWEEPTIME);

        //printf("LoadTest::Tester::finish_tests_loop(): waking up\n");
    }
}

void LoadTest::Tester::finish_tests_finish ()
{
    int ret = pthread_join(finish_tests_thread, NULL);

    if (ret != 0) {
        printf("LoadTest::Tester::finish_tests_finish(): ERROR joining finisher thread: [%d:%d]%s\n", ret, errno, strerror(errno));
    }
}
