#ifndef _TEST_H_
#define _TEST_H_

#include "tcp.h"
#include "utils.h"
#include <pthread.h>

namespace LoadTest
{
    struct Input
    {
        sockaddr_in* target;
        CharBuffer* request;

        unsigned int num_sessions;
        unsigned int run_time;

        Input ();

        void print ();
    };

    struct Result
    {
        unsigned int num_sessions;
        unsigned int num_failures;
        unsigned int num_timeouts;
        unsigned long num_bytes_written;
        unsigned long num_bytes_read;
        unsigned int num_premature_shutdowns;
        double time;

        Result ();

        void print ();
    };

    class Test
    {
        private:
        pthread_t the_thread;

        public:
        TcpRunStatus last_run;
        Input* input;
        Result result;

        Test ();

        void start ();
        void wait_and_finish ();

        private:
        void _run ();
        static void* _start_thread (void*);
    };
}

#endif
