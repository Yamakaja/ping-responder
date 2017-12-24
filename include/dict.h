#ifndef HEADER_LIST
#define HEADER_LIST

#include <unistd.h>

struct mc_client;

struct dict {
    struct mc_client **data;
    size_t capacity;
};

typedef struct dict dict;

dict dict_init(size_t initial_capacity);
void dict_cleanup(dict *);

struct mc_client **dict_get(dict *, size_t position);

#endif

