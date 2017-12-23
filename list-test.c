#include <stdio.h>

#include "list.h"

void list_print(struct int_list *list) {
    printf("{size: %ld, capacity: %ld, contents: [", list->size, list->capacity);
    for (int i = 0; i < list->size; i++)
        printf("%d ", list->data[i]);
    puts("]}");
}

int main(int argc, char **argv) {
    struct int_list list = list_init(8);

    while (1) {
        char mode = getchar();
        int i;
        scanf("%d", &i);
        getchar();
        
        if (mode == 'r')
            list_remove(&list, i);
        else if (mode == 'a')
            list_add(&list, i);
        else
            puts("Unknown mode!");

        list_print(&list);
    }
}

