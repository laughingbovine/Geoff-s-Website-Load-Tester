#ifndef _UTILS_H_
#define _UTILS_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/time.h>

#define READ_LINES_END ".\n" // line to search for to indicate read_lines() should stop reading

void error (const char *fmt, ...);
char* read_lines (char* read_buffer, const int read_buffer_size);
char* read_file (const char* filename, char* read_buffer, const int read_buffer_size);
char* string_copy (char* string);
double stopwatch (struct timeval** lasttime_p);
void print_timeval (struct timeval* tv);

#endif
