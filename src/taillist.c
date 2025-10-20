/*
 * Copyright (C) 2001, 2002 Red Hat Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#include "taillist.h"


/*
 * Code
 */

/* The function predicts the item belongs to the list. It should be used only in the loop */
GList *
tail_list_remove (TailList *list, GList* item)
{
  GList *next;

  /* Is the item the tail in the list? */
  if ( list->tail == item )
    {
      /* Then we need to update the 'tail' vector to the previous item. Then we
       * will remove the item from the list. If the list consists of one item only
       * then both vectors are set to null by the g_list_delete_link and because
       * previous item of deleted one is null */
      list->tail = item->prev;
    }

  /* Okay, the item is not the tail in the list, the possibility the list
   * consists of one item only is handled in the condition above, so we don't
   * need to worry about the 'tail' vector and we simply destroy the item */
  next = item->next;
  list->items = g_list_delete_link (list->items, item);
  return next;
}

void
tail_list_add (TailList *list, gpointer data)
{
  GList *item, *tail;

  if (list->items == NULL)
    {
      /* When we add new item to an empty list, the first item is the tail at the same time */
      list->items = g_list_prepend (NULL, data);
      list->tail = list->items;
    }
  else
    {
      tail = list->tail;
 
      /* Create new item */
      item = g_list_alloc ();
      item->data = data;
      item->prev = tail;
      item->next = NULL;

      /* Let's add new item after the tail and update the pointer to the last then */
      tail->next = item;
      list->tail = item;
    }
}

void
tail_list_concat (TailList *list1, GList *list2)
{
  GList *newlist;

  newlist = g_list_copy (list2);
  if ( newlist != NULL )
    {
      list1->items = g_list_concat (list1->items, newlist);
      list1->tail = g_list_last (newlist);
    }
}

#if !GLIB_CHECK_VERSION(2,28,0)

static void
foreach_hash_table_free (gpointer key, gpointer value, gpointer data)
{
  g_free (key);
  g_free (value);
}

void
free_hash_table (GHashTable *hash_table)
{
  g_hash_table_foreach (hash_table, foreach_hash_table_free, NULL );
  g_hash_table_destroy (hash_table);
}

static void
list_foreach_free (gpointer data, gpointer user_data)
{
    g_free (data);
}

void
free_list (GList *list)
{
  g_list_foreach (list, list_foreach_free, NULL);
  g_list_free (list);
}

#endif // GLIB_CHECK_VERSION
