/*
   Linked list module
 
   Copyright (c) Thomas Pongrac
 
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
 
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
*/

#include <stdlib.h>
#include <stdio.h>
#include "sd_linked_list.h"
#include "sd_dynamic_memory.h"
#include "sd_error.h"

void linked_list_init(linked_list *list)
{
  list->top        = NULL;
  list->list.next  = NULL;
}

list_item *linked_list_add(linked_list *list, void *value)
{
  /* first item */
  if (!list->top)
  {
    list->list.value = value;
    list->top = &list->list;
  }
  /* we allready have items */
  else
  {
    SAFE_CALLOC(list->top->next, 1, sizeof(struct list_item));
    list->top = list->top->next;
    list->top->value = value;
    list->top->next = NULL;
  }

  /* address of added item */
  return list->top;
}

int linked_list_rem(linked_list *list, list_item *item,
        int dalloc)
{
  list_item *c_item, *p_item, *t_item;
  
  c_item = &list->list;
  p_item = NULL;

  /* no items to remove */
  if (!list->top)
    return -1;

  for (;;)
  {
    /* found item to remove */
    if (c_item == item)
    {
      /* deallocate memory in value */
      if (dalloc)
        SAFE_FREE(c_item->value);

      /* first item (don't free)*/
      if (!p_item)
      {
        /* shift 2nd to 1st */
	      if (c_item != list->top)
        {
          c_item->value = c_item->next->value;
	        /* if 3rd exists update */
          t_item = c_item->next;
          if (c_item->next != list->top)
	        {
            c_item->next = c_item->next->next;
	        }
	        else
	        {
            list->top = c_item;
	          c_item->next = NULL;
          }
          c_item = t_item;
        }

        /* only 1 item */
	      else
        {
          list->top = p_item;
          c_item->next = NULL;
          break;
        }
      }
      else
      {
        /* set new top */
        if (c_item == list->top)
          list->top = p_item;
      }
      

      /* remove the unwanted list entry */
      if (p_item)
        p_item->next = c_item->next;

      free(c_item);

      break;
    }

    /* reached the end */
    else if (c_item == list->top)
    {
      return -1;
    }

    p_item = c_item;
    c_item = c_item->next;
  }

  return 0;
}

int linked_list_get_size(linked_list *list)
{
  int i;
  list_item *c_item;

  i = 1;
  c_item = &list->list;

  /* no items */
  if (!list->top)
    return 0;

  while (c_item != list->top)
  {
    c_item = c_item->next;
    i++;
  }

  return i;
}

list_item *linked_list_iterate(linked_list *list,
        int (*iterate_func) (void *value, int index))
{
  int i;
  list_item *c_item;

  i = 1;
  c_item = &list->list;

  /* no items */
  if (!list->top)
    return NULL;

  for (;;)
  {
    if ((*iterate_func)(c_item->value, i))
      return c_item;

    if (c_item == list->top)
      break;

    c_item = c_item->next;
    i++;
  }

  return NULL;
}

int linked_list_rem_all_entries(linked_list *list, int dalloc)
{
  int nrem = 0;

  /* keep removing first till empty */
  while (linked_list_rem(list, &list->list, dalloc) != -1)
  {
    nrem++;
  }
  return nrem;
}

int linked_list_deinit_rem_all_entries(linked_list *list, int dalloc,
    void (*cleanup)(void *v))
{
  int size, i;
  void **ent;

  size = linked_list_get_all_values(list, &ent);

  if (!size)
    return 0;

  for (i = 0; i < size; i++)
    cleanup(ent[i]);
  
  /* keep removing first till empty */
  while (linked_list_rem(list, &list->list, dalloc) != -1) ;

  return size;
}

int linked_list_get_all_values(linked_list *list, void ***vlist)
{
  int i, size;
  list_item *c_item;

  c_item = &list->list;
  size = linked_list_get_size(list);
  if (!size)
    return 0;

  SAFE_CALLOC(*vlist, size, sizeof(void *));


  for (i = 0; i < size; i++)
  {
    *((*vlist) + i)  = c_item->value;
    c_item = c_item->next;
  }
  
  return size;
}

// vim:ts=2:expandtab
