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

#ifndef _FLAG_H_
#define _FLAG_H_

#include <glib.h>


typedef enum {
    LIBS_l       = (1 << 0),
    LIBS_L       = (1 << 1),
    LIBS_OTHER   = (1 << 2),
    CFLAGS_I     = (1 << 3),
    CFLAGS_OTHER = (1 << 4),
    LIBS_ANY     = (LIBS_l | LIBS_L | LIBS_OTHER),
    CFLAGS_ANY   = (CFLAGS_I | CFLAGS_OTHER),
    FLAGS_ANY    = (LIBS_ANY | CFLAGS_ANY)
} FlagType;

#if GLIB_CHECK_VERSION(2,28,0)
  #define flag_free_list(l)   g_list_free_full ((l), (GDestroyNotify) flag_free)
#else
  void flag_free_list (GList *list);
#endif // GLIB_CHECK_VERSION

typedef struct
{
  FlagType type;
  char *arg;
} Flag;


void flag_free (Flag *flag);
Flag * flag_create (FlagType type, char *arg);

GList * flag_merge_lists (GList *packages, FlagType type);
char * flag_list_to_string (GList *list);
GList * flag_list_strip_duplicates (GList *list);

char * flags_packages_get (GList *pkgs, FlagType flags);
char * flag_get_multi_merged (GList *pkgs, FlagType type, gboolean in_path_order, gboolean include_private);


#endif  /* _FLAG_H_ */
