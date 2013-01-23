#include "utils.h"

bool Global::premature_shutdown = false;

////////////////////////////////////////////////////////////////////////////////

void warn (const char *fmt, ...)
{
    int retval;
    char* str;

    va_list ap;
    va_start(ap, fmt);
    retval = vasprintf(&str, fmt, ap);
    va_end(ap);

    if (retval > 0)
    {
        fprintf(stderr, "%s\n", str);

        free(str);
    }
}

void error (const char *fmt, ...)
{
    int retval;
    char* str;

    va_list ap;
    va_start(ap, fmt);
    retval = vasprintf(&str, fmt, ap);
    va_end(ap);

    if (retval > 0)
    {
        fprintf(stderr, "%s\n", str);

        free(str);
    }

    exit(1);
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

CharBuffer::CharBuffer (int starting_size) :
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
    int num_bytes = 0;

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

    if (input.bad())
        error("problem reading istream");
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

void CharBuffer::set_size (int new_size)
{
    if (new_size <= 0)
    {
        size = 0;

        if (chars != NULL)
        {
            free(chars);
            chars = NULL;
        }
    }
    else // new_size > 0
    {
        size = new_size;

        // yes, i know sizeof(char) == 1
        if (chars == NULL)
            chars = (char*)malloc(size * sizeof(char));
        else
            chars = (char*)realloc(chars, size * sizeof(char));
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
