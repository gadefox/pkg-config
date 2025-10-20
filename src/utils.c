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

#include <stdio.h>

#include "utils.h"
#include "globals.h"
#include "package.h"


/*
 * Code
 */

gboolean
define_global_variable (const char *varname,
                        const char *varval)
{
  char *newname;
  char *newval;

  if ( globals == NULL )
    /* We don't loopup the entry because hash table is empty */
    globals = create_hash_table (g_str_hash, g_str_equal );
  else if ( g_hash_table_lookup (globals, varname) != NULL )
    {
      verbose_error ("Variable '%s' defined twice globally\n", varname);
      return FALSE;
    }

  newname = g_strdup (varname);
  newval = g_strdup (varval);

  g_hash_table_insert (globals, newname, newval);

  debug_spew ("Global variable definition '%s' = '%s'\n",
              varname, varval);

  return TRUE;
}

/* List of flag pointers */
GList *
fill_list (GList *packages, FlagType type,
           gboolean in_path_order, gboolean include_private)
{
  GList *iter;
  GList *expanded = NULL;
  GList *flags;
  GHashTable *visited;

  /* Start from the end of the requested package list to maintain order since
   * the recursive list is built by prepending. */
  visited = g_hash_table_new (g_str_hash, g_str_equal);

  for (iter = g_list_last (packages); iter != NULL; iter = iter->prev )
    {
      recursive_fill_list (iter->data, include_private, visited, &expanded);
    }

  g_hash_table_destroy (visited);
  package_spew_list ("post-recurse", expanded);

  if (in_path_order)
    {
      package_spew_list ("original", expanded);
      expanded = packages_sort_by_path_position (expanded);
      package_spew_list ("  sorted", expanded);
    }

  flags = flag_merge_lists (expanded, type);
  g_list_free (expanded);

  return flags;
}

static int
pathposcmp (gconstpointer a, gconstpointer b)
{
  const Package *pa = a;
  const Package *pb = b;

  if (pa->path_position < pb->path_position)
    return -1;

  if (pa->path_position > pb->path_position)
    return 1;

  return 0;
}

GList *
packages_sort_by_path_position (GList *list)
{
  return g_list_sort (list, pathposcmp);
}

/* Construct a topological sort of all required packages.
 *
 * This is a depth first search starting from the right.  The output 'listp' is
 * in reverse order, with the first node reached in the depth first search at
 * the end of the list.  Previously visited nodes are skipped.  The result is
 * a list of packages such that each packages is listed once and comes before
 * any package that it depends on.
 *
 * listp is a list of package pointers
 */
void
recursive_fill_list (Package *pkg, gboolean include_private,
                     GHashTable *visited, GList **listp)
{
  GList *iter;

  /*
   * If the package has already been visited, then it is already in 'listp' and
   * we can skip it. Additionally, this allows circular requires loops to be
   * broken.
   */
  if ( g_hash_table_lookup_extended (visited, pkg->key, NULL, NULL) )
    {
      debug_spew ("Package %s already in requires chain, skipping\n",
                  pkg->key);
      return;
    }

  /* Record this package in the dependency chain, so add new key to hash table */
  g_hash_table_replace (visited, pkg->key, pkg->key);

  /* Start from the end of the required package list to maintain order since
   * the recursive list is built by prepending. */
  iter = include_private ? pkg->requires_private.items : pkg->requires.items;

  for ( iter = g_list_last (iter); iter != NULL; iter = iter->prev )
    {
      recursive_fill_list (iter->data, include_private, visited, listp);
    }

  *listp = g_list_prepend (*listp, pkg);
}

char *
file_build_path (const char *dir, const char *name)
{
    return g_strdup_printf ("%s%c%s.pc", dir, G_DIR_SEPARATOR, name);
}

char *
file_find_in_search_dirs (const char *name, unsigned int *path_position)
{
  GList *iter;
  unsigned int position = 0;
  char *location;

  for ( iter = search_dirs.items; iter != NULL; iter = iter->next )
    {
      position++;
      location = file_build_path ((char *) iter->data, name);

      if (g_file_test (location, G_FILE_TEST_IS_REGULAR))
        {
          *path_position = position;
          return location;
        }

      g_free (location);
    }

    return NULL;
}

void
release ( void )
{
  if ( globals != NULL )
    free_hash_table (globals);

  if ( packages != NULL )
    package_free_hash_table (packages );

  free_list (search_dirs.items);
  free_list (cflag_system_dirs.items);
  free_list (lib_system_dirs.items);
}

void
die (int status)
{
  /* release allocated memory */
  release ( );

  /* terminate app */
  exit (status);
}

gboolean
scan_dirs (Package *pkg_config)
{
  GList *iter;
  static gboolean initted = FALSE;

  if ( initted )
    return TRUE;

  initted = TRUE;

  for ( iter = search_dirs.items; iter != NULL; iter = iter->next )
    {
      if ( !scan_dir ((char *) iter->data, pkg_config) )
        return FALSE;
    } 

  return TRUE;
}

/* Look for .pc files in the given directory and add them into
 * locations, ignoring duplicates
 */
gboolean
scan_dir (const char *dirname, Package *pkg_config)
{
  GDir *dir;
  const gchar *filename;
  unsigned int length;
  char *tmpname;
  gboolean result = TRUE;

  /*
   * Use a copy of dirname cause Win32 opendir doesn't like
   * superfluous trailing (back)slashes in the directory name.
   */
  tmpname = g_strdup (dirname);
  length = strlen (tmpname);

  if (length != 0 && tmpname [length - 1] == G_DIR_SEPARATOR)
    {
      length--;
      tmpname [length] = '\0';
    }

#ifdef G_OS_WIN32
  /* Turn backslashes into slashes or
   * g_shell_parse_argv() will eat them when ${prefix}
   * has been expanded in parse_libs().
   */
  backslash_to_slash (dirname);
#endif

  dir = g_dir_open (tmpname, 0, NULL);
  if ( dir == NULL )
    {
      debug_spew ("Cannot open directory '%s' in package search path: %s\n",
                  tmpname, g_strerror (errno));
      goto quit;
    }

  debug_spew ("Scanning directory '%s'\n", tmpname);
  for ( ;; )
    {
      filename = g_dir_read_name (dir);
      if ( filename == NULL )
        break;

      if ( !scan_file (filename, tmpname, pkg_config) )
        {
          result = FALSE;
          goto quit;
        }
    }

quit:

  g_free (tmpname);

  if ( dir != NULL )
      g_dir_close (dir);

  return result;
}

gboolean
scan_file (const char *filename, const char *dirname, Package *pkg_config)
{
  char *path;
  Package *pkg;
  gboolean die;
 
#if HAVE_PARSE_SPEW
  parse_spew ("  file>%s\n", filename);
#endif

  path = g_build_filename (dirname, filename, NULL);
  pkg = package_get (pkg_config, path, FALSE, disable_uninstalled, &die);
  g_free (path);

  /* Please note we don't destroy package item structure because it's
   * added in hash table already so we don't loose memory
   */

  return pkg != NULL || !die;
}

gboolean
init_pc_path (void)
{
#ifdef G_OS_WIN32
  char *instdir, *lpath, *shpath;

  instdir = g_win32_get_package_installation_directory_of_module (NULL);
  if (instdir == NULL)
    {
      /* This only happens when GetModuleFilename() fails. If it does, that
       * failure should be investigated and fixed.
       */
      debug_spew ("g_win32_get_package_installation_directory_of_module failed\n");
      return FALSE;
    }

  lpath = g_build_filename (instdir, "lib", "pkgconfig", NULL);
  shpath = g_build_filename (instdir, "share", "pkgconfig", NULL);
  pkg_config_pc_path = g_strconcat (lpath, G_SEARCHPATH_SEPARATOR_S, shpath,
                                    NULL);
  g_free (instdir);
  g_free (lpath);
  g_free (shpath);
#else
  pkg_config_pc_path = PKG_CONFIG_PC_PATH;
#endif

  return TRUE;
}

void
add_search_dir (const char *path, const char *source)
{
  char *newdir;

  debug_spew ("Adding directory '%s' from %s\n", path, source);

  newdir = g_strdup (path);
  tail_list_add (&search_dirs, newdir);
}

void
add_search_dirs (const char *path, const char *separator, const char *source)
{
  char **split_dirs;
  char **iter;
  char *dir;

  split_dirs = g_strsplit (path, separator, -1);

  for ( iter = split_dirs, dir = *iter; dir != NULL; dir = *++iter )
    {
      add_search_dir (dir, source);
    }

  g_strfreev (split_dirs);
}

static void
internal_spew (const char *format, va_list args, gboolean use_stdout)
{
  gchar *str;
  FILE* stream;

  g_return_if_fail (format != NULL);

  if (use_stdout)
    stream = stdout;
  else
    stream = stderr;

  str = g_strdup_vprintf (format, args);

  fputs (str, stream);
  fflush (stream);

  g_free (str);
}

void
spew (const char *format, ...)
{
  va_list args;

  va_start (args, format);
  internal_spew (format, args, FALSE);
  va_end (args);
}

void
debug_spew (const char *format, ...)
{
  va_list args;

  if (!want_debug_spew)
    return;

  va_start (args, format);
  internal_spew (format, args, want_stdout_errors);
  va_end (args);
}

void
verbose_error (const char *format, ...)
{
  va_list args;

  if (!want_verbose_errors)
    return;

  va_start (args, format);
  internal_spew (format, args, want_stdout_errors);
  va_end (args);
}

#if HAVE_PARSE_SPEW

void
parse_spew (const char *format, ...)
{
  va_list args;

  if (!want_parse_spew)
    return;

  va_start (args, format);
  internal_spew (format, args, want_stdout_errors);
  va_end (args);
}

#endif  /* HAVE_PARSE_SPEW */
