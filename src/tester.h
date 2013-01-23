#ifndef _TESTER_H_
#define _TESTER_H_

#include <vector>
#include "test.h"

void start_premature_shutdown (int param);

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

        // results
        unsigned int total_sessions;
        unsigned int total_failures;
        unsigned int total_timeouts;
        unsigned long total_bytes_written;
        unsigned long total_bytes_read;
        unsigned int total_premature_shutdowns;
        unsigned int tests_ignored;
        double total_time;

        public:
        Tester (const char*, int, unsigned int, unsigned int);
        ~Tester ();

        void add_input (CharBuffer&);

        void set_num_tests_per_ramp (unsigned int);
        void set_num_ramps (unsigned int);
        void set_ramp_wait (unsigned int);

        void print_initial_summary ();
        void start_signal_handlers ();
        void init_tests ();
        void run_tests ();
        void finish_tests ();
        void print_final_summary ();

        // TODO: test finisher thread
    };
};

#endif
