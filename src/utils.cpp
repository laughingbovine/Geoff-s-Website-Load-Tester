#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"

bool Global::premature_shutdown = false;

////////////////////////////////////////////////////////////////////////////////

//void print_stack_trace_and_exit (int sig)
//{
//    void *array[50];
//    size_t size;
//
//    // get void*'s for all entries on the stack
//    size = backtrace(array, 50);
//
//    // print out all the frames to stderr
//    fprintf(stderr, "Error: signal %d:\n", sig);
//    backtrace_symbols_fd(array, size, 2);
//    exit(1);
//}
//
//void warn (const char *fmt, ...)
//{
//    int retval;
//    char* str;
//
//    va_list ap;
//    va_start(ap, fmt);
//    retval = vasprintf(&str, fmt, ap);
//    va_end(ap);
//
//    if (retval > 0)
//    {
//        fprintf(stderr, "%s\n", str);
//
//        free(str);
//    }
//    else
//    {
//        fprintf(stderr, "warn(): sorry... %s", fmt);
//    }
//}
//
//void error (const char *fmt, ...)
//{
//    int retval;
//    char* str;
//
//    va_list ap;
//    va_start(ap, fmt);
//    retval = vasprintf(&str, fmt, ap);
//    va_end(ap);
//
//    if (retval > 0)
//    {
//        fprintf(stderr, "%s\n", str);
//
//        free(str);
//    }
//    else
//    {
//        fprintf(stderr, "error(): sorry... %s", fmt);
//    }
//
//    exit(1);
//}

void print_pthread_t (pthread_t& pt)
{
    unsigned char *ptc = (unsigned char*)(void*)(&pt);
    printf("0x");
    for (size_t i=0; i<sizeof(pt); i++) {
        printf("%02x", (unsigned)(ptc[i]));
    }
}

////////////////////////////////////////////////////////////////////////////////

Stopwatch::Stopwatch ()
{
    gettimeofday(&this_time, NULL);

    last_time = this_time;
}

double Stopwatch::lap ()
{
    double result;

    // cycle times
    last_time = this_time;
    gettimeofday(&this_time, NULL);

    // result is the difference
    result = (double)(this_time.tv_sec - last_time.tv_sec) +
             ((double)(this_time.tv_usec - last_time.tv_usec) / 1000000);

    return result;
}

////////////////////////////////////////////////////////////////////////////////

CharBuffer::CharBuffer () :
    chars(NULL)
{
    set_size(CHARBUFFER_DEFAULT_SIZE);
}

CharBuffer::CharBuffer (unsigned int starting_size) :
    chars(NULL)
{
    set_size(starting_size);
}

CharBuffer::CharBuffer (char const* starting_string) :
    chars(NULL)
{
    set_size(strlen(starting_string));

    memcpy(chars, starting_string, size);
}

CharBuffer::CharBuffer (istream &input) :
    chars(NULL)
{
    unsigned int num_bytes = 0;

    set_size(CHARBUFFER_DEFAULT_SIZE);

    while (input.good())
    {
        input.read(chars + num_bytes, size - num_bytes);

        num_bytes += input.gcount();

        if (input.eof())
            set_size(num_bytes);
        else
            set_size(num_bytes + CHARBUFFER_DEFAULT_SIZE);
    }

    if (input.bad()) {
        printf("problem reading istream\n");
        exit(1);
    }
}

CharBuffer::CharBuffer (const CharBuffer& other) :
    chars(NULL)
{
    set_size(other.size);

    memcpy(chars, other.chars, size);
}

CharBuffer::~CharBuffer ()
{
    set_size(0);
}

char* CharBuffer::let_go ()
{
    char* chars2 = chars;

    size = 0;
    chars = NULL;

    return chars2;
}

void CharBuffer::set_size (unsigned int new_size)
{
    if (new_size <= 0) {
        size = 0;

        if (chars != NULL) {
            delete [] chars; //free(chars);
            chars = NULL;
        }
    } else { // new_size > 0
        if (chars != NULL) {
            char* new_chars = new char[new_size];
            memcpy(new_chars, chars, (size > new_size ? new_size : size) * sizeof(char));
            delete [] chars;
            chars = new_chars;
        } else {
            chars = new char[new_size];
        }

        size = new_size;
    }
}

ostream& operator<< (ostream& output, CharBuffer &b)
{
    output.write(b.chars, b.size);

    return output;
}

////////////////////////////////////////////////////////////////////////////////

//CharBuffer* read_istream (istream &input, CharBuffer &b)
//{
//    int num_bytes = 0, last_read_size = 0;
//
//    CharBuffer* result = new CharBuffer(0);
//
//    while (input.good())
//    {
//        input.read(b.chars, b.size);
//
//        last_read_size = input.gcount();
//
//        result->set_size(num_bytes + last_read_size);
//
//        memcpy(result->chars + num_bytes, b.chars, last_read_size);
//
//        num_bytes += last_read_size;
//    }
//
//    if (input.bad())
//    {
//        cerr << "problem reading input stream" << endl;
//
//        result = NULL;
//    }
//
//    return result;
//}
//
//CharBuffer* read_file (const char* filename, CharBuffer &b)
//{
//    ifstream input(filename);
//
//    return read_istream(input, b);
//}

////////////////////////////////////////////////////////////////////////////////

//CharBuffer* sread_istream (istream &input, CharBuffer &b)
//{
//    int num_bytes = 0, last_read_size = 0;
//
//    CharBuffer* result = new CharBuffer(0);
//
//    while (input.good())
//    {
//        input.get(b.chars, b.size, '\0');
//
//        last_read_size = input.gcount();
//
//        result->set_size(num_bytes + last_read_size + 1);
//
//        memcpy(result->chars + num_bytes, b.chars, last_read_size);
//
//        num_bytes += last_read_size;
//    }
//
//    result->chars[num_bytes + 1] = '\0'; // set that null char
//
//    if (input.bad())
//    {
//        cerr << "problem reading input stream" << endl;
//
//        result = NULL;
//    }
//
//    return result;
//}
//
//CharBuffer* sread_file (const char* filename, CharBuffer &b)
//{
//    ifstream input(filename);
//
//    return sread_istream(input, b);
//}
