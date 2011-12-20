#include "utils.h"

void error (const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);

    fprintf(stderr, "\n");

    exit(1);
}

char* read_lines (char* read_buffer, const int read_buffer_size)
{
    // yes, i know sizeof(char) == 1, just getting into the habit

    char* result = calloc(1, sizeof(char)); // initialized to null char

    while (!feof(stdin))
    {
        fgets(read_buffer, read_buffer_size, stdin);

        if (ferror(stdin))
            break;

        result = realloc(result, (strlen(result) + strlen(read_buffer) + 1) * sizeof(char));
        result = strcat(result, read_buffer);
    }

    if (ferror(stdin))
        error("read_lines(): error reading from stdin");

    return result;
}

char* read_file (const char* filename, char* read_buffer, const int read_buffer_size)
{
    FILE* fp;                               // the file object pointer
    char* result = calloc(1, sizeof(char)); // file's contents, initialized to null char

    fp = fopen(filename, "r");

    //fprintf(stderr, "opening file '%s'\n", filename);

    if (fp == NULL)
        error("read_file(): error opening file");

    while (!feof(fp))
    {
        fgets(read_buffer, read_buffer_size, fp);

        if (ferror(fp))
            break;

        result = realloc(result, (strlen(result) + strlen(read_buffer) + 1) * sizeof(char));
        result = strcat(result, read_buffer);
    }

    if (ferror(fp))
        error("read_file(): error reading file");

    //fprintf(stderr, "file %s:\n---\n%s---\n", filename, result);

    return result;
}

char* string_copy (char* string)
{
    char* result;

    result = malloc((strlen(string) + 1) * sizeof(char));

    return strcpy(result, string);
}

double stopwatch (struct timeval** lasttime_p)
{
    // ugh, this gets ugly with the pointers

    double result;
    struct timeval *thistime = malloc(sizeof(struct timeval));

    gettimeofday(thistime, NULL);

    if (*lasttime_p == NULL)
    {
        result = 0.0;
    }
    else
    {
        //printf("((*thistime).tv_sec - (**lasttime_p).tv_sec) + (((*thistime).tv_usec - (**lasttime_p).tv_usec) / 1000000)\n");
        //printf("(%d - %d) + ((%d - %d) / 1000000)\n", (int)(*thistime).tv_sec, (int)(**lasttime_p).tv_sec, (int)(*thistime).tv_usec, (int)(**lasttime_p).tv_usec);

        result = (double)((*thistime).tv_sec - (**lasttime_p).tv_sec) +
                 ((double)((*thistime).tv_usec - (**lasttime_p).tv_usec) / 1000000);

        //printf("last time: ");
        //print_timeval(*lasttime_p);
        //printf("this time: ");
        //print_timeval(thistime);

        free(*lasttime_p);
    }

    *lasttime_p = thistime;

    //printf("result: %.5f\n", result);

    return result;
}

void print_timeval (struct timeval* tv)
{
    if (tv != NULL)
        printf("timeval has %d seconds and %d milliseconds\n", (int)tv->tv_sec, (int)tv->tv_usec);
    else
        printf("d'oh\n");
}
