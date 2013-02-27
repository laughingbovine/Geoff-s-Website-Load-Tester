#ifndef _TEST_H_
#define _TEST_H_

#include "tcp.h"
#include "utils.h"
#include <pthread.h>
#include <signal.h>

#define TEST_FAILURE_SLEEPTIME 1

#define TEST_TESTSTATUS_SIZE 8

enum TestStatus {
    JUST_CREATED,       // 0
    STARTING,           // 1
    CREATE_THREAD_FAIL, // 2
    RUNNING,            // 3
    DONE_RUNNING,       // 4
    JOINING,            // 5
    JOIN_THREAD_FAIL,   // 6
    JOINED              // 7
};

extern const char* TestStatusStrings [TEST_TESTSTATUS_SIZE];

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
        unsigned int stats [TCP_TCPRUNSTATUS_SIZE];
        unsigned int num_sessions;
        unsigned int num_failures;
        unsigned int num_connect_attempts;
        unsigned int num_connect_successes;
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
        unsigned int id;
        pthread_t the_thread;
        pthread_mutex_t the_mutex;
        TestStatus status;

        TcpRun* run;
        TcpRunStatus last_run;
        unsigned int consecutive_failures;
        Stopwatch clock;

        public:
        Input* input;
        Result result;

        Test (unsigned int);
        ~Test ();

        void set_status (TestStatus);
        TestStatus get_status ();

        void start ();
        void wait_and_finish ();

        void print ();

        private:
        static void* _start_thread (void*);
        void _run ();
    };
}

#endif
