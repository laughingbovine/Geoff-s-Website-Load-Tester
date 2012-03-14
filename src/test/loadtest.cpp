#include "utils.h"
#include "tcp.h"
#include "test.h"
#include <pthread.h>
#include <unistd.h>
//#include <limits.h>

#define BUF_SIZE 1024

int i, ret;
//char buffer[BUF_SIZE]; // general-use buffer
CharBuffer buffer;

// the following variables are set from command-line args
int     cli_num_concurrent_users    = 1;
int     cli_num_sessions            = 0;
int     cli_run_time_seconds        = 0;
char    *cli_host_name              = NULL;
int     cli_port_number             = 0;

int     cli_num_input_files         = 0;
char**  cli_input_files             = NULL;

void usage ()
{
    fprintf(stderr, "usage:\n");
    fprintf(stderr, "loadtest -h host_name -p port_number\n");
    fprintf(stderr, "         [-c num_concurrent_users]\n");
    fprintf(stderr, "         [-r num_sessions]\n");
    fprintf(stderr, "         [-t run_time_seconds]\n");
    fprintf(stderr, "         [-i input_file1 -i input_file2 -i input_file3 ...]\n");
    exit(1);
}

void get_options (int argc, char **argv)
{
    int opt;

    // first, need to know how many -i arguments there are so we can allocate for them all
    for (i = 0; i < argc; i++)
        if (strncmp(*(argv+i), "-i", 3) == 0)
            cli_num_input_files++;

    cli_input_files = (char**)calloc(cli_num_input_files, sizeof(char*));

    // grab options
    i = 0; // use this for the input file counter
    while ((opt = getopt (argc, argv, "c:h:i:p:r:t:")) != -1)
    {
        switch (opt)
        {
            case 'c':
                cli_num_concurrent_users = atoi(optarg);
                break;

            case 'h':
                cli_host_name = optarg;
                break;

            case 'i': // can accept multiple inputs
                cli_input_files[i] = optarg;
                i++;
                break;

            case 'p':
                cli_port_number = atoi(optarg);
                break;

            case 'r':
                cli_num_sessions = atoi(optarg);
                break;

            case 't':
                cli_run_time_seconds = atoi(optarg);
                break;

            default:
                usage();
        }
    }

    // enforce usage
    if (cli_num_concurrent_users < 1 || cli_host_name == NULL || cli_port_number < 1)
        usage();
}

void print_initial_summary ()
{
    //printf("maximum number of threads: %ld or %d\n", sysconf(_SC_THREADS), _POSIX_THREAD_THREADS_MAX);

    if (cli_num_concurrent_users > 1)
        printf("running with %d users, ", cli_num_concurrent_users);
    else
        printf("running with 1 user, ");

    if (cli_num_sessions > 0 && cli_run_time_seconds > 0)
        printf("each doing %d sessions or running for %d seconds (whichever comes first), ", cli_num_sessions, cli_run_time_seconds);
    else if (cli_num_sessions > 0)
        printf("each doing %d sessions, ", cli_num_sessions);
    else if (cli_run_time_seconds > 0)
        printf("each running for %d seconds, ", cli_run_time_seconds);
    else
        printf("each doing 1 session, ");

    printf("targeting %s port %d...\n", cli_host_name, cli_port_number);
}

int main (int argc, char **argv)
{
    //test_input      *the_inputs;
    //pthread_t       *the_tests;
    //test_results    **the_results;

    get_options(argc, argv);

    // allocate input buckets, threads, and result buckets
    //the_inputs  = calloc(cli_num_input_files > 0 ? cli_num_input_files : 1, sizeof(test_input));
    //the_tests   = calloc(cli_num_concurrent_users, sizeof(pthread_t));
    //the_results = calloc(cli_num_concurrent_users, sizeof(test_results*));
    LoadTest::Input     the_inputs  [cli_num_input_files > 0 ? cli_num_input_files : 1];
    LoadTest::Test      the_tests   [cli_num_concurrent_users];
    LoadTest::Results   the_results [cli_num_concurrent_users];

    unsigned long total_bytes   = 0;
    unsigned int total_timeouts = 0;
    unsigned int total_sessions = 0;
    double total_time           = 0;

    // initialize a test input
    the_inputs[0].host_name         = cli_host_name;
    the_inputs[0].port_number       = cli_port_number;
    the_inputs[0].num_sessions      = cli_num_sessions;
    the_inputs[0].run_time_seconds  = cli_run_time_seconds;
    the_inputs[0].input             = "";

    // grab input
    if (cli_num_input_files > 0) // input coming from files designated using command line args
    {
        // first, copy the first test input to all other test inputs
        for (i = 1; i < cli_num_input_files; i++)
            the_inputs[i] = the_inputs[0];
            //memcpy(&the_inputs[i], &the_inputs[0], sizeof(test_input));

        // then, assign each input file to each of the_inputs
        for (i = 0; i < cli_num_input_files; i++)
            the_inputs[i].input = *(read_file(cli_input_files[i], buffer));
    }
    else
    {
        printf("Please enter the message: \n");
        the_inputs[0].input = *(read_istream(cin, buffer));
        printf("Thank you.\n");

        cli_num_input_files = 1;
    }

    print_initial_summary();

    // create all those threads, assigning a different the_inputs to each test by cycling through all of them
    for (i = 0; i < cli_num_concurrent_users; i++)
        the_tests[i].start(the_inputs[i%cli_num_input_files]);
        //if (ret = pthread_create(&the_tests[i], NULL, run_test, (void*)( &the_inputs[i%cli_num_input_files] )))
        //    error("main(): ERROR creating test thread, %d", ret);

    // now join them all back and do some metrics
    for (i = 0; i < cli_num_concurrent_users; i++)
    {
        // the pointer magic gets confusing, but it works
        the_results[i] = the_tests[i].wait_and_finish();
        //pthread_join(the_tests[i], (void*)&the_results[i]);

        total_bytes     += the_results[i].num_bytes;
        total_timeouts  += the_results[i].num_timeouts;
        total_time      += the_results[i].time;
        total_sessions  += the_results[i].num_sessions;

        // thread summary
        //printf("thread %d completed %u sessions read %.2f MiB in %.2f seconds (%.2f KiBps) (%u timeouts)\n",
        //    i,
        //    the_results[i]->num_sessions,
        //    (double)the_results[i]->num_bytes / (1024 * 1024),
        //    the_results[i]->time,
        //    the_results[i]->num_bytes / (the_results[i]->time * 1024),
        //    the_results[i]->num_timeouts);

        //free(the_results[i]);
    }

    // free all allocated memory
    //for (i = 0; i < cli_num_input_files; i++)
    //    free(the_inputs[i].input);

    //free(the_inputs);
    //free(the_tests);
    //free(the_results);

    // overall summary
    printf("Totals:\n");
    printf("===================\n");
    printf("Sessions:               %-8u\n",                                    total_sessions);
    printf("                        %-8.2f sessions per second\n",              total_sessions / (total_time / cli_num_concurrent_users));
    printf("                        %-8.2f sessions per thread\n",              (double)total_sessions / cli_num_concurrent_users);
    printf("                        %-8.2f sessions per thread per second\n",   total_sessions / total_time);
    printf("Total Time:             %-8.2f seconds\n",                          total_time);
    printf("                        %-8.2f average seconds per thread\n",       total_time / cli_num_concurrent_users);
    printf("Data Read:              %-8.2f MiB\n",                              (double)total_bytes / (1024 * 1024));
    printf("Data Transfer Rate:     %-8.2f average MiBps\n",                    (total_bytes / (1024 * 1024)) / (total_time / cli_num_concurrent_users));
    printf("                        %-8.2f average KiBps per thread\n",         (total_bytes / 1024) / total_time);
    printf("Number of Timeouts:     %-8u\n",                                    total_timeouts);

    return 0;
}
