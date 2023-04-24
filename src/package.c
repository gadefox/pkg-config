/*
 * Copyright (C) 2001, 2002 Red Hat Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
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

#include "package.h"
#include "cflags.h"
#include "globals.h"
#include "libs.h"
#include "parse.h"
#include "reqver.h"
#include "strutil.h"
#include "utils.h"


/*
 * Code
 */

static void
package_free_list_foreach (gpointer data, gpointer user_data)
{
  package_free (data);
}

void
package_free_list (GList *list)
{
  g_list_foreach (list, package_free_list_foreach, NULL);
  g_list_free (list);
}

void
package_free (Package *pkg)
{
  g_free (pkg->orig_prefix);
  g_free (pkg->key);
  g_free (pkg->name);
  g_free (pkg->version);
  g_free (pkg->description);
  g_free (pkg->url);
  g_free (pkg->pcfiledir);

  required_version_free_list (pkg->requires_entries);
  required_version_free_list (pkg->requires_private_entries);
  required_version_free_list (pkg->conflicts);

  flag_free_list (pkg->libs.items);
  flag_free_list (pkg->cflags.items);

  g_list_free (pkg->requires.items);
  g_list_free (pkg->requires_private.items);

  /* This hash table has minimum one entry so no need to check */
  if ( pkg->vars != NULL )
    free_hash_table (pkg->vars);

  if ( pkg->required_versions != NULL )
    g_hash_table_destroy (pkg->required_versions);

  g_free (pkg);
}

static gint
packages_sort_cb (gconstpointer a,
                  gconstpointer b)
{
  const Package *package_a = *((Package **) a);
  const Package *package_b = *((Package **) b);

  return g_strcmp0 (package_a->key, package_b->key);
}

void
package_print_list (Package *config)
{
  int mlen = 0;
  GPtrArray *packages_array = NULL;
  GHashTableIter iter;
  gpointer key, value;
  guint i;

  ignore_requires = TRUE;
  ignore_requires_private = TRUE;

  /* Add the packages to a pointer array and sort by pkg->key first, to give
   * deterministic output. While doing that, work out the maximum key length
   * so we can pad the output correctly. */
  packages_array = g_ptr_array_sized_new (g_hash_table_size (packages));
  g_hash_table_iter_init (&iter, packages);

  while ( g_hash_table_iter_next (&iter, &key, &value) )
    {
      g_ptr_array_add (packages_array, value);
      mlen = MAX (mlen, strlen (key));
    }

  g_ptr_array_sort (packages_array, packages_sort_cb);

  for ( i = 0; i < packages_array->len; i++ )
    {
        Package *pkg = g_ptr_array_index (packages_array, i);
        char *pad;

        pad = g_strnfill (mlen + 1 - strlen (pkg->key), ' ');

        printf ("%s%s%s - %s\n",
                pkg->key, pad, pkg->name, pkg->description);

        g_free (pad);
    }

  g_ptr_array_free (packages_array, TRUE);
}

char *
packages_get_var (Package    *config,
                  GList      *pkgs,
                  const char *varname)
{
  GList *iter;
  GString *str;
  Package *pkg;
  char *var;

  str = g_string_new (NULL);

  for ( iter = pkgs; iter != NULL; iter = iter->next )
    {
      pkg = iter->data;

      var = parse_package_variable (pkg, config, varname);
      if (var == NULL)
        continue;

      if (str->len > 0)
        g_string_append_c (str, ' ');

      g_string_append (str, var);
      g_free (var);
    }

  return g_string_free (str, FALSE);
}

char *
package_get_var_globals (Package *pkg,
                        Package *config,
                        const char *var)
{
  char *varval;
  char *temp_var;
  const char *temp_var_content;

  if (globals != NULL)
    {
      varval = g_hash_table_lookup (globals, var);
      if (varval != NULL)
      {
        debug_spew ("Overriding variable '%s' from global list\n", var);
        return g_strdup(varval);
      }
    }

  /* Allow overriding specific variables using an environment variable of the
   * form PKG_CONFIG_$PACKAGENAME_$VARIABLE
   */
  if (pkg->key != NULL)
    {
      /* env variable */
      temp_var = var_to_env_var (pkg->key, var);
      temp_var_content = getenv (temp_var);
      g_free (temp_var);

      if (temp_var_content != NULL)
        {
          debug_spew ("Overriding variable '%s' with '%s' from environment\n", var, temp_var_content);
          return g_strdup (temp_var_content);
        }

      /* pkg-config package variable
       * Please note the package is created after reading pc file
       */
      if ( config != NULL && config->vars != NULL )
        {
          temp_var = var_to_pkg_config_var (pkg->key, var);
          temp_var_content = g_hash_table_lookup (config->vars, temp_var);
          g_free (temp_var);

          if (temp_var_content != NULL)
            {
              debug_spew ("Overriding variable '%s' with '%s' from pkg-config package\n", var, temp_var_content);
              return g_strdup (temp_var_content);
            }
        }
    }

  if (pkg->vars != NULL)
  {
    varval = g_hash_table_lookup (pkg->vars, var);
    return g_strdup (varval);
  }

  return NULL;
}

gboolean
package_get_varval_bool (Package *pkg,
                         const char *var)
{
  char *varval;

  varval = package_get_var (pkg, var);
  if (varval == NULL)
    return FALSE;

  /* '1' */
  if ( is_str_one_text (varval) )
    return TRUE;

  /* 'true' */
  return is_str_true_text (varval);
}

static gboolean
package_verify_required_fields (Package *pkg)
{
  if (pkg->key == NULL)
    {
      spew ("Internal pkg-config error, package with no key, please file a bug report\n");
      return FALSE;
    }

  if (pkg->name == NULL)
    {
      verbose_error ("Package '%s' has no Name: field\n",
                     pkg->key);
      return FALSE;
    }

  if (pkg->version == NULL)
    {
      verbose_error ("Package '%s' has no Version: field\n",
                     pkg->key);
      return FALSE;
    }

  if (pkg->description == NULL)
    {
      verbose_error ("Package '%s' has no Description: field\n",
                     pkg->key);
      return FALSE;
    }

  return TRUE;
}

/* Oh my.. another spaghetti code. Now it's better ,) */
static gboolean
package_verify (Package *pkg, Package *config)
{
  /* Be sure we have the required fields */
  if ( !package_verify_required_fields( pkg ) )
    return FALSE;

  /* Make sure we have the right version for all requirements */
  if ( !package_verify_requires_private (pkg) )
    return FALSE;

  /* Make sure we didn't drag in any conflicts via Requires
   * (inefficient algorithm, who cares)
   */
  if ( !package_verify_required (pkg) )
    return FALSE;

  /* We make a list of system directories that compilers expect so we
   * can remove them.
   */

   cflags_verify (pkg, config);
   libs_verify (pkg, config);

   return TRUE;
}

gboolean
package_verify_required (Package *pkg)
{
  GList *requires = NULL;   /* List of package pointers */
  GHashTable *visited;      /* Hash table of ??? pointers */
  GList *iter;

  /* Make sure we didn't drag in any conflicts via Requires
   * (inefficient algorithm, who cares)
   */
  visited = g_hash_table_new (g_str_hash, g_str_equal);
  recursive_fill_list (pkg, TRUE, visited, &requires);
  g_hash_table_destroy (visited);

  for ( iter = requires; iter != NULL; iter = iter->next )
    {
      if ( package_verify_required_item (pkg, iter->data) )
        continue;

      g_list_free (requires);
      return FALSE;
    }

  g_list_free (requires);

  return TRUE;
}

gboolean
package_verify_requires_private (Package *pkg)
{
  GList *iter;
  Package *req;
  RequiredVersion *ver;

  /* No need to ask this in the 'for' loop */
  if (pkg->required_versions == NULL)
    return TRUE;

  /* Make sure we have the right version for all requirements */
  for ( iter = pkg->requires_private.items; iter != NULL; iter = iter->next )
    {
      req = iter->data;

      ver = g_hash_table_lookup (pkg->required_versions, req->key);
      if ( ver == NULL )
        continue;

      if (version_test (ver->comparison, req->version, ver->version))
        continue;

      verbose_error ("Package '%s' requires '%s %s %s' but version of %s is %s\n",
                     pkg->key, req->key,
                     comparison_to_str (ver->comparison),
                     ver->version,
                     req->key,
                     req->version);

      if (req->url)
        verbose_error ("You may find new versions of %s at %s\n",
                       req->name, req->url);

      return FALSE;
    }

   return TRUE;
}

gboolean
package_verify_required_item (Package *pkg, Package *req)
{
  GList *iter;
  RequiredVersion *ver;

  for ( iter = pkg->conflicts; iter != NULL; iter = iter->next )
    {
      ver = iter->data;

      if (strcmp (ver->name, req->key) == 0 &&
          version_test (ver->comparison, req->version, ver->version))
        {
          verbose_error ("Version %s of %s creates a conflict.\n"
                         "(%s %s %s conflicts with %s %s)\n",
                         req->version, req->key,
                         ver->name,
                         comparison_to_str (ver->comparison),
                         ver->version ? ver->version : "(any)",
                         ver->owner->key,
                         ver->owner->version);

          return FALSE;
        }
    }

  return TRUE;
}

char *
package_get_var (Package *pkg,
                 const char *var)
{
  char *varval;

  if ( pkg->vars == NULL )
    return NULL;

  varval = g_hash_table_lookup (pkg->vars, var);
  if ( varval == NULL )
    return NULL;

  if ( *varval == '\0' )
    return NULL;

  return varval;
}

void
package_spew_list (const char *name,
                   GList     *list)
{
  GList *iter;
  Package *pkg;

  debug_spew ("  package>%s:", name);

  for ( iter = list; iter != NULL; iter = iter->next )
    {
      pkg = iter->data;

      debug_spew (" [key=%s]", pkg->key);
    }

  debug_spew ("\n");
}

static
const char * def_name = "pkg-config";

Package *
package_get_pkgconfig (gboolean *die)
{
  Package *pkg_config;
  char *def_path;

  def_path = file_build_path (PKG_CONFIG_PACKAGE_PATH, def_name);

  debug_spew ("Reading pkg-config package: '%s'\n", def_path);
  pkg_config = parse_package_file (def_name, def_path, NULL, TRUE, TRUE, TRUE, die);
  if (pkg_config == NULL)
    {
      debug_spew ("Failed to parse '%s'\n", def_path);

      g_free (def_path);
      return NULL;
    }

  g_free (def_path);

  if (!package_verify_required_fields (pkg_config))
      goto quit;

  if (!version_test (EQUAL, pkg_config->version, VERSION))
    {
      verbose_error ("Package version ('%s') does not match with pkg-config version ('%s')\n", pkg_config->version, VERSION);
      goto quit;
    }

  /* debug_spew */
  if (package_get_varval_bool (pkg_config, "debug"))
    {
      enable_debug_spew ( );
      debug_spew ("'debug' variable enabling debug spew\n");
    }

#if HAVE_PARSE_SPEW
  /* parse_spew */
  if (package_get_varval_bool (pkg_config, "parse"))
    {
      enable_parse_spew ( );
      debug_spew ("'parse' variable enabling parse spew\n");
    }
#endif // HAVE_PARSE_SPEW

  /* pc_path */
  pkg_config_pc_path = package_get_var( pkg_config, "pc_path" );
  if ( pkg_config_pc_path == NULL )
    {
      verbose_error ("Package does not containt 'pc_path' variable.\n");

      if ( !package_add_pcpath(pkg_config) )
        goto quit;
    }

  return pkg_config;

quit:

  package_free (pkg_config);

  *die = FALSE;
  return NULL;
}

static Package *
package_create (Package *pkg_config, const char *name, gboolean warn, gboolean ignore_uninstalled, gboolean *die)
{
  Package *pkg;
  char *key;
  char *location;
  unsigned int path_position;
  char *un;

  debug_spew ("Looking for package '%s'\n", name);

  /* treat "name" as a filename if it ends in .pc and exists */
  if ( ends_in_dotpc (name) )
    {
      debug_spew ("Considering '%s' to be a filename rather than a package name\n", name);

      location = g_strdup (name);

      /* need to strip package name out of the filename */
      key = g_path_get_basename (name);
      key[strlen (key) - EXT_LEN] = '\0';

      path_position = 0;
    }
  else
    {
      /* See if we should auto-prefer the uninstalled version */
      if (!ignore_uninstalled &&
          !name_ends_in_uninstalled (name))
        {
          un = g_strconcat (name, "-uninstalled", NULL);

          pkg = package_get (pkg_config, un, FALSE, TRUE, die);

          g_free (un);

          if (pkg != NULL)
            {
              debug_spew ("Preferring uninstalled version of package '%s'\n", name);
              return pkg;
            }

          /* Package is null, but we didn't terminate the application using exit (die)
           * so we'll check if we can continue in the process
           */
          if ( *die )
            {
              /* We don't need to free anything in the function */
              return NULL;
            }
        }

      location = file_find_in_search_dirs( name, &path_position );
      if ( location == NULL )
        {
          if ( warn )
            verbose_error ("Package %s was not found in the pkg-config search path.\n"
                           "Perhaps you should add the directory containing `%s.pc'\n"
                           "to the PKG_CONFIG_PATH environment variable\n",
                           name, name);

          /* We don't need to free anything in the function */
          *die = FALSE;
          return NULL;
        }

      key = g_strdup (name);
    }

  debug_spew ("Reading '%s' from file '%s'\n", name, location);

  pkg = parse_package_file (key, location, pkg_config, ignore_requires,
                            ignore_private_libs, ignore_requires_private, die);

  g_free (key);

  if (pkg == NULL)
    {
      debug_spew ("Failed to parse '%s'\n", location);

      g_free (location);

      *die = FALSE;
      return NULL;
    }

  if (strstr (location, "uninstalled.pc"))
    pkg->uninstalled = TRUE;

  g_free (location);

  pkg->path_position = path_position;

  debug_spew ("Path position of '%s' is %d\n", pkg->key, pkg->path_position);

  /* We have to add the package before pulling package requests! */
  packages_add (pkg);

  return pkg;
}

Package *
package_get (Package *pkg_config, const char *name, gboolean warn, gboolean ignore_uninstalled, gboolean *die)
{
  Package *pkg;

  /* Package hash table has been created by the initialize funtion */
  pkg = g_hash_table_lookup (packages, name);
  if (pkg != NULL)
    return pkg;

  pkg = package_create (pkg_config, name, warn, ignore_uninstalled, die );
  if ( pkg == NULL )
    return NULL;

  /* pull in Requires packages */
  if ( !package_pull_request (pkg, pkg_config, warn, ignore_uninstalled, die) )
    goto quit;

  /* pull in Requires.private packages */
  if ( !package_pull_request_private (pkg, pkg_config, warn, ignore_uninstalled, die) )
    goto quit;

  /* make requires_private include a copy of the public requires too */
  tail_list_concat( &pkg->requires_private, pkg->requires.items );

  if ( !package_verify (pkg, pkg_config) )
  {
    *die = FALSE;
    goto quit;
  }

  return pkg;

quit:

  g_hash_table_remove (packages, pkg->key);

#if !GLIB_CHECK_VERSION(2,28,0)
  package_free (pkg);
#endif

  return NULL;
}

gboolean
package_pull_request (Package *pkg, Package *config, gboolean warn, gboolean ignore_uninstalled, gboolean *die)
{
  GList *iter;
  RequiredVersion *ver;
  Package *req;

  for (iter = pkg->requires_entries; iter != NULL; iter = iter->next)
    {
      ver = iter->data;

      debug_spew ("Searching for '%s' requirement '%s'\n", pkg->key, ver->name);

      req = package_get (config, ver->name, warn, ignore_uninstalled, die);
      if (req == NULL)
        {
          verbose_error ("Package '%s', required by '%s', not found\n", ver->name, pkg->key);

          /* We don't terminate the application using exit (die), but we
           * release allocated memory for the package item and we don't continue
           * in the process */
          return FALSE;
        }

      required_versions_add (pkg, ver);
      tail_list_add (&pkg->requires, req );
    }

    return TRUE;
}

gboolean
package_pull_request_private (Package *pkg, Package *config, gboolean warn, gboolean ignore_uninstalled, gboolean *die)
{
  GList *iter;
  RequiredVersion *ver;
  Package *req;

  for ( iter = pkg->requires_private_entries; iter != NULL; iter = iter->next )
    {
      ver = iter->data;

      debug_spew ("Searching for '%s' private requirement '%s'\n", pkg->key, ver->name);

      req = package_get (config, ver->name, warn, ignore_uninstalled, die);
      if (req == NULL)
        {
          verbose_error ("Package '%s', required by '%s', not found\n", ver->name, pkg->key);

          /* We don't terminate the application using exit (die), but we
           * release allocated memory for the package item and we don't continue
           * in the process */
          return FALSE;
        }

      required_versions_add (pkg, ver);
      tail_list_add (&pkg->requires_private, req);
    }

  return TRUE;
}


#if !GLIB_CHECK_VERSION(2,28,0)

static void
package_free_foreach (gpointer key, gpointer value, gpointer data)
{
  package_free( value );
}

void
package_free_hash_table (GHashTable *hash_table)
{
  g_hash_table_foreach (hash_table, package_free_foreach, NULL );
  g_hash_table_destroy (hash_table);
}

#endif // GLIB_CHECK_VERSION

void
packages_add(Package *pkg)
{
  debug_spew ("Adding '%s' package to list of known packages\n", pkg->key);
  g_hash_table_insert (packages, pkg->key, pkg);
}

Package *
package_create_virtual_pkgconfig (void)
{
  Package *pkg_config;

  /* There is no package file installed so let's create new virtual package */
  debug_spew ("Creating virtual pkg-config package\n");

  pkg_config = g_new0 (Package, 1);
  pkg_config->key = g_strdup (def_name);
  pkg_config->version = g_strdup (VERSION);
  pkg_config->name = g_strdup (def_name);
  pkg_config->description = g_strdup ("System package that allow querying of the compiler and linker flags");
  pkg_config->url = g_strdup ("http://pkg-config.freedesktop.org");
  pkg_config->virtual = TRUE;

  /* Get the built-in search path */
  if ( !package_add_pcpath( pkg_config ) )
    {
      package_free (pkg_config);
      return NULL;
    }

  return pkg_config;
}

void
package_add_var (Package *pkg,
                const char  *var,
                const char  *val)
{
  char *newvar;
  char *newval;

  if (pkg->vars == NULL)
    pkg->vars = g_hash_table_new (g_str_hash, g_str_equal);

  newvar = strdup (var);
  newval = strdup (val);

  g_hash_table_insert (pkg->vars, (gpointer*) newvar,
                       (gpointer*) newval);
}

gboolean
package_add_pcpath (Package *pkg_config)
{
  /* Get the built-in search path */
  if ( !init_pc_path( ) )
    return FALSE;

  package_add_var( pkg_config, "pc_path", pkg_config_pc_path);

  return TRUE;
}

/* See if > 0 pkgs were uninstalled */
gboolean
package_uninstalled (Package *pkg)
{
  GList *iter;

  if (pkg->uninstalled)
    return TRUE;

  for ( iter = pkg->requires.items; iter != NULL; iter = iter->next )
    {
      pkg = iter->data;

      if (package_uninstalled (pkg))
        return TRUE;
    }

  return FALSE;
}

char *
package_trim_and_sub (Package *pkg, Package *config, const char *str, const char *path)
{
  char *trimmed;
  GString *subst;
  char *p;
  char c;
  char *temp;
  char *var_name;

  trimmed = s_trim (str);
  subst = g_string_new ("");

  for ( p = trimmed, c = *p++; c != '\0'; c = *p++ )
    {
      if (c != '$')
        {
          g_string_append_c (subst, c);
          continue;
        }

      /* Get next character */
      c = *p++;

      /* Check for end of string */
      if (c == '\0')
        {
          /* Don't forget the '$' character */
          g_string_append_c (subst, '$');
          break;
        }

      /* Check for "$$" */
      if (c == '$')
        {
          /* escaped $ */
          g_string_append_c (subst, '$');
          continue;
        }

      /* Check for "${" */
      if (c == '{')
        {
          temp = p;

          /* Get up to close brace. */
          p = s_end_bracket (p);
          var_name = g_strndup (temp, p - temp);
          p++;  /* Past brace */

          /* We don't need this variable */
          temp = package_get_var_globals (pkg, config, var_name);

          if (temp == NULL)
            {
              verbose_error ("Variable '%s' not defined in '%s'\n",
                             var_name, path);

              if (parse_strict)
                {
                  g_free (var_name);
                  goto quit;
                }

              /* We can't append 'temp' value, because it's null, see bellow. */
              g_string_append (subst, var_name);
            }
          else
            {
              /* Bugfix: This function call has to be here due to GLIB's
               * critical assert message. Explanation: When 'parse_strict'
               * is false we don't terminate the application, but the
               * 'temp' is still null. */
              g_string_append (subst, temp);
              g_free (temp);
            }

          g_free (var_name);
          continue;
        }

      /* Don't forget the '$' character */
      g_string_append_c (subst, '$');
      g_string_append_c (subst, c);
    }

  g_free (trimmed);
  p = subst->str;
  g_string_free (subst, FALSE);

  return p;

quit:

  g_free (trimmed);
  g_string_free (subst, TRUE);

  return NULL;
}

Package *
packages_initialize (void)
{
  Package *pkg_config;
  gboolean die;

  /* Init global variables */
  packages = package_create_hash_table (g_str_hash, g_str_equal);

  /* Try to load pkg-config package file otherwise create a virtual package */
  pkg_config = package_get_pkgconfig( &die );
  if ( pkg_config == NULL )
    {
      /* We didn't terminate the application using 'exit' (die) so we can't
       * continue in the process
       */
      if (die)
        return NULL;

      pkg_config = package_create_virtual_pkgconfig ( );
    }

  /* env variables */
  cflag_init_system_dirs (pkg_config);
  lib_init_system_dirs (pkg_config);

  packages_add (pkg_config);

  return pkg_config;
}
