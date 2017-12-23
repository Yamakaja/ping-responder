#include "list.h"
#include <stdlib.h>

struct int_list list_init(size_t initial_capacity) {
    struct int_list new_list;
    new_list.min_capacity = initial_capacity;
    new_list.data = malloc(sizeof(int) * initial_capacity);
    new_list.capacity = initial_capacity;
    new_list.size = 0;
    return new_list;
}

int list_resize(struct int_list *list, size_t new_size) {
    int *tmp = realloc(list->data, sizeof(int) * new_size);
    if (tmp == NULL)
        return -1;
    
    list->data = tmp;
    return 0;
}

int list_add(struct int_list *list, int value) {
    if (list->size == list->capacity) {
        if (list_resize(list, list->capacity * 2))
            return -1;
        
        list->capacity *= 2;
    }

    list->data[list->size++] = value;
    return 0;
}

int list_remove(struct int_list *list, int value) {
    int i = 0;
    for (; i < list->size; i++)
        if (list->data[i] == value)
            break;

    if (list->size == i)
        return -1;

    list->data[i] = list->data[list->size - 1];
    list->size--;
    return 0;
}

