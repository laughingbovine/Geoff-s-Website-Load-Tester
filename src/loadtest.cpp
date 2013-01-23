//#include <pthread.h>
//#include <unistd.h>
//#include <limits.h>
//#include <math.h>
//#include <signal.h>
#include <iostream>
#include <fstream>
//#include "utils.h"
//#include "tcp.h"
//#include "test.h"
#include "tester.h"

// the following variables are set from command-line args (in get_options())
int     cli_num_concurrent_users    = 1;
int     cli_ramp_factor             = 1;
int     cli_ramp_wait               = 1;
int     cli_num_sessions            = 0;
int     cli_run_time_seconds        = 0;
char    *cli_host_name              = NULL;
int     cli_port_number             = 0;
int     cli_num_input_files         = 0;
char    **cli_input_files           = NULL;

void usage ()
{
    fprintf(stderr, "usage:\n");
    fprintf(stderr, "loadtest -h host_name -p port_number\n");
    fprintf(stderr, "         [-c num_concurrent_users]\n");
    fprintf(stderr, "         [-r num_sessions]\n");
    fprintf(stderr, "         [-t run_time_seconds]\n");
    fprintf(stderr, "         [-R ramp_factor]\n");
    fprintf(stderr, "         [-W ramp_wait (in seconds)]\n");
    fprintf(stderr, "         [-i input_file1 -i input_file2 -i input_file3 ...]\n");
    exit(1);
}

void get_options (int argc, char **argv)
{
    int opt;

    // first, need to know how many -i arguments there are so we can allocate for them all
    for (int i = 0; i < argc; i++)
        if (strncmp(*(argv+i), "-i", 3) == 0)
            cli_num_input_files++;

    cli_input_files = (char**)calloc(cli_num_input_files, sizeof(char*));

    // grab options
    int i = 0; // use this for the input file counter
    while ((opt = getopt (argc, argv, "c:h:i:p:r:t:R:W:")) != -1)
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

            case 'R':
                cli_ramp_factor = atoi(optarg);
                break;

            case 'W':
                cli_ramp_wait = atoi(optarg);
                break;

            default:
                usage();
        }
    }

    // enforce usage
    if (cli_num_concurrent_users < 1 || cli_host_name == NULL || cli_port_number < 1 || cli_ramp_factor < 1 || cli_ramp_wait < 0)
    {
        printf("bad arguments?\n");

        usage();
    }
}

int main (int argc, char **argv)
{
    // process some input
    get_options(argc, argv);

    // grab input(s)
    CharBuffer** requests;

    if (cli_num_input_files > 0) // input coming from files designated using command line args
    {
        requests = new CharBuffer*[cli_num_input_files];

        // then, assign each input file to each of the_inputs
        for (int i = 0; i < cli_num_input_files; i++)
        {
            ifstream input(cli_input_files[i]);

            requests[i] = new CharBuffer(input);
        }
    }
    else
    {
        cli_num_input_files = 1;

        requests = new CharBuffer*[1];

        printf("Please enter the request:\n");
        requests[0] = new CharBuffer(cin);
    }

    // set up the tester
    LoadTest::Tester t(cli_host_name, cli_port_number, cli_num_sessions, cli_run_time_seconds);

    t.set_num_tests_per_ramp(cli_num_concurrent_users);
    t.set_num_ramps(cli_ramp_factor);
    t.set_ramp_wait(cli_ramp_wait);

    for (int i = 0; i < cli_num_input_files; i++)
        t.add_input(*requests[i]);

    // do it!
    t.print_initial_summary();
    t.start_signal_handlers();
    t.init_tests();
    t.run_tests();
    t.finish_tests();
    t.print_final_summary();

    // clean up
    for (int i = 0; i < cli_num_input_files; i++)
        delete requests[i];
    delete [] requests;

    return 0;
}
