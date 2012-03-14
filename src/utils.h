#ifndef _UTILS_H_
#define _UTILS_H_

#include <iostream>
#include <fstream>
#include <sys/time.h>

using namespace std;

void error (const char *fmt, ...);

class Stopwatch
{
    private:
    struct timeval this_time, last_time;

    public:
    Stopwatch ();
    double lap ();
};

#define CHARBUFFER_DEFAULT_SIZE 1024

struct CharBuffer
{
    char* chars;
    int size;

    CharBuffer ();
    CharBuffer (int);
    CharBuffer (char const*);
    ~CharBuffer ();

    char* let_go ();
    void set_size (int);

    friend ostream& operator<< (ostream& output, CharBuffer &b);
};

CharBuffer* read_istream (istream &input, CharBuffer &b);
CharBuffer* read_file (const char* filename, CharBuffer &b);
CharBuffer* sread_istream (istream &input, CharBuffer &b);
CharBuffer* sread_file (const char* filename, CharBuffer &b);

#endif
