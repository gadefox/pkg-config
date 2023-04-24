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

#ifndef _UTILS_H_
#define _UTILS_H_

#include "package.h"
#include "flag.h"


typedef enum
{
  Error,
  Success,
  Die,
  IgnoreUninstalled
} Result;

typedef struct
{
  Package *config;
  gboolean success;
} ForeachScandir;


gboolean define_global_variable (const char *varname, const char *varval);

GList * fill_list (GList *packages, FlagType type, gboolean in_path_order, gboolean include_private);
GList * packages_sort_by_path_position (GList *list);

void recursive_fill_list (Package *pkg, gboolean include_private, GHashTable *visited, GList **listp);

char * file_build_path (const char *dir, const char *name);
char * file_find_in_search_dirs ( const char *name, unsigned int *path_position );

void release (void);
void die (int status);

gboolean scan_dirs (Package *pkg_config);
gboolean scan_dir (const char *dirname, Package *pkg_config);
gboolean scan_file (const char *filename, const char *dirname, Package *pkg_config);

gboolean init_pc_path ();

void add_search_dir (const char *path, const char *source);
void add_search_dirs (const char *path, const char *separator, const char *source);

#if HAVE_PARSE_SPEW
  void parse_spew (const char *format, ...);
#endif

void spew (const char *format, ...);
void debug_spew (const char *format, ...);
void verbose_error (const char *format, ...);
void verbose_warn (const char *format, ...);


#endif  /* _UTILS_H_ */
