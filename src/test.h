#ifndef _TEST_H_
#define _TEST_H_

#include "tcp.h"
#include "utils.h"
#include <pthread.h>

namespace LoadTest
{
    struct Input
    {
        //Input () { cout << "input init" << endl; }
        const char* host_name;
        int port_number;
        CharBuffer* input;
        int num_sessions;
        int run_time_seconds;
    };

    struct Results
    {
        //Results () { cout << "results init" << endl; }
        double time;
        unsigned long num_bytes_read;
        unsigned long num_bytes_written;
        unsigned int num_timeouts;
        unsigned int num_sessions;
    };

    class Test
    {
        private:
        pthread_t the_thread;

        private:
        static void* _run (void* test_input);

        public:
        void start (Input &i);
        Results& wait_and_finish ();

    };
}

#endif
