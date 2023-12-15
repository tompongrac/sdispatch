#ifndef SD_LINKED_LIST_H
#define SD_LINKED_LIST_H

#include <stdlib.h>

/*! \brief An item of a linked list */
struct list_item
{
  void                *value;
  struct list_item    *next;

};
typedef struct list_item list_item;

/*! \brief A linked list */
typedef struct
{
  list_item           *top;
  list_item           list;

} linked_list;

/*! \brief Initialise a linked list */
extern void linked_list_init(linked_list *list);

/*! \brief Add item to linked list */
extern list_item *linked_list_add(linked_list *list, void *value);

/*! \brief Remove item from linked list */
extern int linked_list_rem(linked_list *list, list_item *item,
        int dalloc);

/*! \brief Get number of items in linked list */
extern int linked_list_get_size(linked_list *list);

/*! \brief Iterate linked list items and call specified callback on each item */
extern list_item *linked_list_iterate(linked_list *list,
        int (*iterate_func) (void *value, int index));

/*! \brief Remove all the items from the linked list */
extern int linked_list_rem_all_entries(linked_list *list, int dalloc);

/*! \brief Call a deinit callback for each of the items from the linked list then remove 
 * them all */
extern int linked_list_deinit_rem_all_entries(linked_list *list, int dalloc,
    void (*cleanup)(void *v));

/*! \brief get an array of void pointers of all the values */
extern int linked_list_get_all_values(linked_list *list, void ***vlist);


#endif
