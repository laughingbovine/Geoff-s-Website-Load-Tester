#ifndef _TEST_H_
#define _TEST_H_

#include <pthread.h>
#include "tcp.h"
#include "utils.h"

struct test_input {
    char* host_name;
    int port_number;
    char* input;
    int num_sessions;
    int run_time_seconds;
};
typedef struct test_input test_input;

struct test_results {
    double time;
    unsigned long num_bytes;
    unsigned int num_timeouts;
    unsigned int num_sessions;
};
typedef struct test_results test_results;

void* run_test (void* _test_input);
void free_test_input (test_input* obj);
void free_test_results (test_results* obj);

#endif
