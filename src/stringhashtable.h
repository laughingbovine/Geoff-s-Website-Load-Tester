#ifndef _STRINGHASHTABLE_H_
#define _STRINGHASHTABLE_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

struct stringhashtable_item {
    char* key;
    void* value;
    struct stringhashtable_item* next;
};
typedef struct stringhashtable_item stringhashtable_item;

struct stringhashtable {
    unsigned long size;
    stringhashtable_item** values;
};
typedef struct stringhashtable stringhashtable;

stringhashtable* stringhashtable_new (unsigned long hash_table_size);
void* stringhashtable_add (stringhashtable* this, char* k, void* v);
void* stringhashtable_get (stringhashtable* this, char* k);
void* stringhashtable_remove (stringhashtable* this, char* k);
void stringhashtable_free (stringhashtable* this);

#endif
