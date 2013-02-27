#ifndef _TESTER_H_
#define _TESTER_H_

#include "test.h"
#include <vector>
#include <pthread.h>
#include <signal.h>

#define TESTER_FINISH_TEST_THREAD_SWEEPTIME 5

void start_premature_shutdown (int sig);

namespace LoadTest
{
    class Tester
    {
        private:
        // inputs
        const char* host_name;
        int port_number;
        sockaddr_in target;
        unsigned int num_sessions;
        unsigned int run_time;

        unsigned int num_tests_per_ramp;
        unsigned int num_ramps;
        unsigned int num_tests;
        unsigned int ramp_wait;

        // the actual stuff
        vector<Input*> inputs;
        vector<Test*> tests;

        pthread_t finish_tests_thread;

        // results
        unsigned int total_stats [TCP_TCPRUNSTATUS_SIZE];
        unsigned int total_sessions;
        unsigned int total_failures;
        unsigned int total_connect_attempts;
        unsigned int total_connect_successes;
        unsigned int total_timeouts;
        unsigned long total_bytes_written;
        unsigned long total_bytes_read;
        unsigned int total_premature_shutdowns;
        double total_time;

        // stats
        unsigned int num_tests_running;
        unsigned int num_tests_finished;
        unsigned int last_num_tests_running;
        unsigned int last_num_tests_finished;

        public:
        Tester (const char*, int, unsigned int, unsigned int);
        ~Tester ();

        void add_input (CharBuffer*);

        void set_num_tests_per_ramp (unsigned int);
        void set_num_ramps (unsigned int);
        void set_ramp_wait (unsigned int);

        void print_initial_summary ();
        void start_signal_handlers ();
        void init_tests ();
        void finish_tests_start ();
        void run_tests ();
        void finish_tests_finish ();
        void print_final_summary ();

        private:
        void finish_test (int);
        void cleanup_bad_test (int);
        static void* finish_tests_init (void*);
        void finish_tests_loop ();
    };
};

#endif
