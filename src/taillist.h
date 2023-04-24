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

#ifndef _TAIL_LIST_H_
#define _TAIL_LIST_H_

#include <glib.h>


#define tail_list_init(s)  { (s).items = NULL; (s).tail = NULL; }


typedef struct
{
  GList *items;
  GList *tail;
} TailList;


#if GLIB_CHECK_VERSION(2,28,0)
  #define free_list(l)                g_list_free_full ((l), (GDestroyNotify) g_free)

  #define create_hash_table(f, e)     g_hash_table_new_full ((f), (e), (GDestroyNotify) g_free, (GDestroyNotify) g_free)
  #define free_hash_table(ht)         g_hash_table_destroy (ht)
#else
  void free_list (GList *list);

  #define create_hash_table(f, e)     g_hash_table_new ((f), (e))
  void free_hash_table (GHashTable *hash_table);
#endif // GLIB_CHECK_VERSION


GList* tail_list_remove (TailList *list, GList *item);
void tail_list_add (TailList *list, gpointer data);
void tail_list_concat (TailList *list1, GList *list2);


#endif  /* _TAIL_LIST_H_ */
