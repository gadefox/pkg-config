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

#ifndef _PACKAGE_H_
#define _PACKAGE_H_

#include <glib.h>
#include "taillist.h"


typedef struct
{
  char *key;  /* filename name */
  char *name; /* human-readable name */
  char *version;
  char *description;
  char *url;
  char *pcfiledir; /* directory it was loaded from */
  GList *requires_entries;           /* list of RequiredVersion items */
  TailList requires;                 /* list of Package pointers */
  GList *requires_private_entries;   /* list of RequiredVersion items */
  TailList requires_private;         /* list of Package pointers */
  TailList libs;                     /* list of Flag items */
  TailList cflags;                   /* list of Flag items */
  GHashTable *vars;                  /* hash from name to strings */
  GHashTable *required_versions;     /* hash from name RequiredVersion and key pointers */
  GList *conflicts;                  /* list of RequiredVersion items */
  gboolean uninstalled; /* used the -uninstalled file */
  gboolean virtual; /* used for want_listing opt */
  int path_position; /* used to order packages by position in path of their .pc file, lower number means earlier in path */
  int libs_num; /* Number of times the "Libs" header has been seen */
  int libs_private_num;  /* Number of times the "Libs.private" header has been seen */
  char *orig_prefix; /* original prefix value before redefinition */
} Package;


#if GLIB_CHECK_VERSION(2,28,0)
  #define package_create_hash_table(f, e)   g_hash_table_new_full ((f), (e), NULL, (GDestroyNotify) package_free)
  #define package_free_hash_table(ht)       g_hash_table_destroy (ht)
#else
  #define package_create_hash_table(f, e)   g_hash_table_new ((f), (e))

  void package_free_hash_table (GHashTable *hash_table);
#endif // GLIB_CHECK_VERSION


void     package_free_list              (GList      *list);
void     package_free                   (Package    *pkg);
Package *package_get                    (Package    *config,
                                        const char  *name,
                                        gboolean    warn,
                                        gboolean    ignore_uninstalled,
                                        gboolean    *die);
gboolean package_get_varval_bool        (Package    *pkg,
                                        const char  *var);
char *   package_get_var                (Package    *pkg,
                                        const char  *var);
char *   package_get_var_globals        (Package    *pkg,
                                        Package     *config,
                                        const char  *var);
void     package_add_var                (Package    *pkg,
                                        const char  *var,
                                        const char  *val);
char *   packages_get_var               (Package    *config,
                                        GList       *pkgs,
                                        const char  *var);

Package * packages_initialize (void);

void package_print_list (Package *config);

Package * package_get_pkgconfig (gboolean *die);
Package * package_create_virtual_pkgconfig (void);

void packages_add (Package *pkg);
gboolean package_add_pcpath (Package *pkg_config);

gboolean package_pull_request (Package *pkg, Package *config, gboolean warn, gboolean ignore_uninstalled, gboolean *die);
gboolean package_pull_request_private (Package *pkg, Package *config, gboolean warn, gboolean ignore_uninstalled, gboolean *die);

gboolean package_verify_required (Package *pkg);
gboolean package_verify_requires_private (Package *pkg);

char * package_trim_and_sub (Package *pkg, Package *config, const char *str, const char *path);
gboolean package_uninstalled (Package *pkg);

void package_spew_list (const char *name, GList *list);
gboolean package_verify_required_item (Package *pkg, Package *req);


#endif  /* _PACKAGE_H_ */
