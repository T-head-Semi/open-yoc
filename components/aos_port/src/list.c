#ifdef __linux__

#include "aos_port/list.h"

void slist_add_tail(slist_t *node, slist_t *head)
{
    while (head->next) {
        head = head->next;
    }

    slist_add(node, head);
}

void slist_del(slist_t *node, slist_t *head)
{
    while (head->next) {
        if (head->next == node) {
            head->next = node->next;
            break;
        }

        head = head->next;
    }
}

int slist_entry_number(slist_t *queue)
{
    int num;
    slist_t *cur = queue;
    for (num=0; cur->next; cur=cur->next, num++)
        ;

    return num;
}

int dlist_entry_number(dlist_t *queue)
{
    int num;
    dlist_t *cur = queue;
    for (num=0; cur->next != queue; cur=cur->next, num++)
        ;

    return num;
}
#endif

