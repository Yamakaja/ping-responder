#include "dict.h"
#include <stdlib.h>

dict dict_init(size_t initial_capacity) {
    struct dict new_dict;
    new_dict.data = calloc(1, sizeof(void *) * initial_capacity);
    new_dict.capacity = initial_capacity;
    return new_dict;
}

int dict_grow(dict *target, size_t required_size) {
    size_t new_size = target->capacity;
    while (new_size < required_size)
        new_size <<= 1;

    void *tmp = realloc(target->data, sizeof(struct mc_client *) * new_size);
    if (tmp == NULL)
        return -1;

    target->capacity = new_size;
    target->data = tmp;

    return 0;
}

struct mc_client **dict_get(dict *target, size_t position) {
    if (position >= target->capacity)
        if (dict_grow(target, position))
            return NULL;

    return target->data + position;
}

void dict_cleanup(dict *target) {
    free(target->data);
}

