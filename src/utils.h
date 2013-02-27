#ifndef _UTILS_H_
#define _UTILS_H_

#include <iostream>
#include <sys/time.h>
#include <errno.h>
#include <pthread.h>

namespace Global
{
    extern bool premature_shutdown;
}

using namespace std;

//void print_stack_trace_and_exit (int sig);

//void warn (const char *fmt, ...);
//void error (const char *fmt, ...);

void print_pthread_t (pthread_t&);

class Stopwatch
{
    private:
    struct timeval this_time, last_time;

    public:
    Stopwatch ();
    double lap ();
};

#define CHARBUFFER_DEFAULT_SIZE 4096

struct CharBuffer
{
    char* chars;
    int size;

    CharBuffer ();
    CharBuffer (int);
    CharBuffer (char const*);
    CharBuffer (istream &input);
    CharBuffer (const CharBuffer&);
    ~CharBuffer ();

    char* let_go ();
    void set_size (int);

    friend ostream& operator<< (ostream& output, CharBuffer &b);
};

//CharBuffer* read_istream (istream &input, CharBuffer &b);
//CharBuffer* read_file (const char* filename, CharBuffer &b);
//CharBuffer* sread_istream (istream &input, CharBuffer &b);
//CharBuffer* sread_file (const char* filename, CharBuffer &b);

#endif
