#include "stringhashtable.h"

// taken from http://www.cse.yorku.ca/~oz/hash.html (djb2)
unsigned long stringhashtable_hash (char *str)
{
    unsigned long hash = 5381;
    int c;

    while (c = *str++)
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

stringhashtable* stringhashtable_new (unsigned long hash_table_size)
{
    unsigned long i;
    stringhashtable* this;
    this = malloc(sizeof(stringhashtable));

    //printf("creating hash of size %lu\n", hash_table_size);

    this->size = hash_table_size;
    this->values = calloc(this->size, sizeof(stringhashtable_item*));

    for (i = 0; i < this->size; i++)
    {
        (this->values)[i] = NULL;
    }

    return this;
}

void* stringhashtable_add (stringhashtable* this, char* k, void* v)
{
    unsigned long lookup_index;
    stringhashtable_item* lookup_value;
    void* old_value = NULL;

    lookup_index = stringhashtable_hash(k) % this->size;

    //printf("key: %s -- lookup index: %lu\n", k, lookup_index);

    // first, find out if there's anything in this bucket already
    if ((this->values)[lookup_index] != NULL)
    {
        // somethin's in the bucket
        // check all already-existing values to see if we have a duplicate key
        lookup_value = (this->values)[lookup_index];

        while (1)
        {
            if (strcmp(lookup_value->key, k) == 0)
            {
                // keys match, return old value and store new one
                old_value = lookup_value->value;
                lookup_value->value = v;
                break;
            }
            else
            {
                // see if there's another value to check
                if (lookup_value->next != NULL)
                {
                    // there is, lets do this again
                    lookup_value = lookup_value->next;
                    // (no break)
                }
                else
                {
                    // no more values, tack this new one on to the end of the chain
                    lookup_value->next = malloc(sizeof(stringhashtable_item));
                    lookup_value = lookup_value->next;

                    lookup_value->key = k;
                    lookup_value->value = v;
                    lookup_value->next = NULL;
                    break;
                }
            }
        }
    }
    else
    {
        // nothing in the bucket
        // add a new value
        lookup_value = malloc(sizeof(stringhashtable_item));
        (this->values)[lookup_index] = lookup_value;

        lookup_value->key = k;
        lookup_value->value = v;
        lookup_value->next = NULL;
    }

    return old_value;
}

void* stringhashtable_get (stringhashtable* this, char* k)
{
    unsigned long lookup_index;
    stringhashtable_item* lookup_value;

    lookup_index = stringhashtable_hash(k) % this->size;

    //printf("key: %s -- lookup index: %lu\n", k, lookup_index);

    // first, check if there's anything in the bucket
    if ((this->values)[lookup_index] != NULL)
    {
        //printf("something in the bucket\n");

        // somethin's in the bucket
        // check all values to see if we have the key
        lookup_value = (this->values)[lookup_index];

        while (1)
        {
            if (strcmp(lookup_value->key, k) == 0)
            {
                //printf("found %s(==%s)!\n", lookup_value->key, k);

                // keys match, return value
                return lookup_value->value;
            }
            else
            {
                // see if there's another value to check
                if (lookup_value->next != NULL)
                {
                    // there is, lets do this again
                    lookup_value = lookup_value->next;
                    // (no return)
                }
                else
                {
                    //printf("couldnt find %s\n", k);

                    // no more values, couldnt find it
                    return NULL;
                }
            }
        }
    }
    else
    {
        //printf("nothing in the bucket\n");

        return NULL;
    }
}

void* stringhashtable_remove (stringhashtable* this, char* k)
{
    unsigned long lookup_index;
    void* removed;
    stringhashtable_item *lookup_value, *previous_lookup_value;

    lookup_index = stringhashtable_hash(k) % this->size;

    //printf("key: %s -- lookup index: %lu\n", k, lookup_index);

    // first, check if there's anything in the bucket
    if ((this->values)[lookup_index] != NULL)
    {
        //printf("something in the bucket\n");

        // somethin's in the bucket
        previous_lookup_value = NULL;
        // check all values to see if we have the key
        lookup_value = (this->values)[lookup_index];

        while (1)
        {
            if (strcmp(lookup_value->key, k) == 0)
            {
                //printf("found %s(==%s)!\n", lookup_value->key, k);

                // break and reform the chain without the current lookup_value

                removed = lookup_value->value;

                if (previous_lookup_value == NULL)
                {
                    (this->values)[lookup_index] = lookup_value->next; // lookup_value->next may be null, that's ok
                }
                else
                {
                    previous_lookup_value->next = lookup_value->next; // lookup_value->next may be null, that's ok
                }

                free(lookup_value);

                // keys match, return value
                return removed;
            }
            else
            {
                // see if there's another value to check
                if (lookup_value->next != NULL)
                {
                    // there is, lets do this again
                    previous_lookup_value = lookup_value;
                    lookup_value = lookup_value->next;
                    // (no return)
                }
                else
                {
                    //printf("couldnt find %s\n", k);

                    // no more values, couldnt find it
                    return NULL;
                }
            }
        }
    }
    else
    {
        //printf("nothing in the bucket\n");

        return NULL;
    }
}

void stringhashtable_free (stringhashtable* this)
{
    int i;
    stringhashtable_item *lookup_value, *lookup_value_next;

    // free all stringhashtable_item's but not their keys or values
    // traverse all buckets
    for (i = 0; i < this->size; i++)
    {
        // check if this bucket has anything
        if((this->values)[i] != NULL)
        {
            // ok, free the stringhashtable_item, but make sure we have a pointer to the next one
            lookup_value = (this->values)[i];

            while (1)
            {
                lookup_value_next = lookup_value->next;

                free(lookup_value);

                if (lookup_value_next != NULL)
                    lookup_value = lookup_value_next;
                else
                    break;
            }
        }
    }

    // all buckets cleared
    free(this);
}

int main (int argc, char** argv)
{
    //char k[20], v[20];
    char* k;
    char* v;
    void* temp;
    stringhashtable* ht = stringhashtable_new(5);

    //StringHashTable $sht = new StringHashTable(5);
    //$sht->add(k, (void*)v);
    //$sht->get(k);

    printf("enter a key and a value (no more than 20 characters)\n");

    while (1)
    {
        k = calloc(20, sizeof(char));
        v = calloc(20, sizeof(char));

        scanf("%s %s", k, v);

        if (strcmp(k, "done") == 0)
            break;
        else
            stringhashtable_add(ht, k, (void*)v);
    }

    printf("now enter a key to see its value\n");

    while (1)
    {
        scanf("%s", k);

        temp = NULL;
        temp = stringhashtable_remove(ht, k);

        if (temp == NULL)
            printf("key: %s, value: [NULL]\n", k);
        else
            printf("key: %s, value: %s\n", k, (char*)temp);
    }

    stringhashtable_free(ht);
}
