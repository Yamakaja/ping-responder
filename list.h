#ifndef HEADER_LIST
#define HEADER_LIST

#include <unistd.h>

struct int_list {
    int *data;
    size_t size;
    size_t capacity;
    size_t min_capacity;
};

struct int_list list_init(size_t initial_capacity);
int list_add(struct int_list *list, int value);
int list_remove(struct int_list *list, int value);

#endif

