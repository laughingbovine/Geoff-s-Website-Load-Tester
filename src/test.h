#ifndef _TEST_H_
#define _TEST_H_

#include "tcp.h"
#include "utils.h"
#include <pthread.h>

enum TestStatus {
    JUST_CREATED,
    STARTING,
    CREATE_THREAD_FAIL,
    RUNNING,
    DONE_RUNNING,
    JOINING,
    JOIN_THREAD_FAIL,
    JOINED
};

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
        unsigned int num_resets;
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
        TestStatus status;

        public:
        TcpRunStatus last_run;
        Input* input;
        Result result;

        Test ();

        TestStatus get_status () const;

        void start ();
        void wait_and_finish ();

        void print ();

        private:
        static void* _start_thread (void*);
        void _run ();
    };
}

#endif
