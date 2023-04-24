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

#include "cflags.h"
#include "globals.h"
#include "flag.h"
#include "strutil.h"
#include "utils.h"


/* Well known compiler include path environment variables. These are
 * used to find additional system include paths to remove. See
 * https://gcc.gnu.org/onlinedocs/gcc/Environment-Variables.html. */
static const gchar *gcc_include_envvars[] = {
  "CPATH",
  "C_INCLUDE_PATH",
  "CPP_INCLUDE_PATH",
  NULL
};

#ifdef G_OS_WIN32
/* MSVC include path environment variables. See
 * https://msdn.microsoft.com/en-us/library/73f9s62w.aspx. */
static const gchar *msvc_include_envvars[] = {
  "INCLUDE",
  NULL
};
#endif


/*
 * Code
 */

gboolean
cflag_is_system_dirs (const char* compare, const char *key, const char *arg)
{
  GList *iter;

  for ( iter = cflag_system_dirs.items; iter != NULL; iter = iter->next )
    {
      if (strcmp (iter->data, compare) != 0)
        continue;

      debug_spew ("Package %s has %s in Cflags\n", key, arg);

      if (allow_system_cflags)
        continue;

      return TRUE;
    }

  return FALSE;
}

void
cflag_init_system_dirs (Package *pkg_config)
{
  const char *search_path;
  gchar **iter;
  const char **include_vars;
  gchar *var;

  /* SystemIncludePath */
  search_path = getenv ("PKG_CONFIG_SYSTEM_INCLUDE_PATH");
  if (search_path == NULL)
    {
      search_path = package_get_var (pkg_config, "system_include_path");
      if (search_path == NULL)
        search_path = PKG_CONFIG_SYSTEM_INCLUDE_PATH;
    }

  cflag_add_system_dirs (search_path);

  /* C_INCLUDE_PATH */
#ifdef G_OS_WIN32
  include_vars = msvc_syntax ? msvc_include_envvars : gcc_include_envvars;
#else
  include_vars = gcc_include_envvars;
#endif

  for ( iter = (gchar**)include_vars, var = *iter; var != NULL; var = *++iter )
    {
      search_path = getenv (var);
      if (search_path != NULL)
        {
          cflag_add_system_dirs (search_path);
          continue;
        }

      var = var_to_pkg_config_var (NULL, var);
      search_path = package_get_var (pkg_config, var);
      g_free (var);

      if (search_path != NULL)
        cflag_add_system_dirs (search_path);
    }
}

void
cflags_verify ( Package *pkg, Package *config )
{
  Flag *flag;
  char *cflags;
  GList *iter;

  for ( iter = pkg->cflags.items; iter != NULL; )
    {
      flag = iter->data;

      if ( !(flag->type & CFLAGS_I) )
        {
          /* Move to the next item in the list */
          iter = iter->next;
          continue;
        }

      /* Handle the system cflags. We put things in canonical
       * -I/usr/include (vs. -I /usr/include) format, but if someone
       * changes it later we may as well be robust.
       *
       * Note that the -i* flags are left out of this handling since
       * they're intended to adjust the system cflags behavior.
       */

      /* Check '-I' first. */
      cflags = flag->arg;
      if ( *cflags++ != '-' || /* An empty string or doesn't begin w/ '-' character */
           *cflags++ != 'I' )  /* "-" string or doesn't begin w/ "-I" */
        {
          /* Move to the next item in the list */
          iter = iter->next;
          continue;
        }

      /* Check the space then */
      if ( *cflags == ' ' )
        cflags++;

      if ( !cflag_is_system_dirs ( cflags, pkg->key, flag->arg ) )
        {
          /* Move to the next item in the list */
          iter = iter->next;
          continue;
        }

      debug_spew ("Removing %s from Cflags for %s\n", flag->arg, pkg->key);

      /* Remove the current item from the list including freeing the memory. Please note the
       * pointer to the list is changed when it's the first item in the list. Then we move to
       * the next item. */
      iter = tail_list_remove (&pkg->cflags, iter);

#if !GLIB_CHECK_VERSION(2,28,0)
      /* Don't forget to destroy flag object */
      free_flag (flag);
#endif
    }
}

void
cflag_add_system_dirs (const gchar *dirs)
{
  gchar **values;
  gchar **iter;
  gchar *val;

  values = g_strsplit (dirs, G_SEARCHPATH_SEPARATOR_S, 0);

  for ( iter = values, val = *iter; val != NULL; val = *++iter)
    {
      val = g_strdup (val);
      tail_list_add (&cflag_system_dirs, val);
    }

  g_strfreev (values);
}

static gboolean
cflags_do_parse (Package *pkg, int argc, char **argv)
{
  int i;
  Flag *flag;
  FlagType type;
  char *tmp, *arg, *p;
  char *newarg;

  for ( i = 0; i < argc; i++ )
    {
      tmp = s_trim (argv[i]);
      if (tmp == NULL)
        return TRUE;

      arg = s_dup_escape_shell (tmp);
      p = arg;
      g_free (tmp);

      if ( p[0] == '-' &&
           p[1] == 'I' )
        {
          p = s_space (p + 2); /* 2 because "-l" */

          type = CFLAGS_I;
          newarg = g_strconcat ("-I", p, NULL);
          g_free (arg);
        }
      else if ( (strcmp ("-idirafter", arg) == 0 ||
                 strcmp ("-isystem", arg) == 0) &&
                 i + 1 < argc )
        {
          tmp = s_trim (argv[++i]);
          if (tmp == NULL)
            {
              g_free (arg);
              return TRUE;
            }

          p = s_dup_escape_shell (tmp);
          g_free (tmp);

          /* These are -I flags since they control the search path */
          type = CFLAGS_I;
          newarg = g_strconcat (arg, " ", p, NULL);
          g_free (p);
          g_free (arg);
        }
      else if (*arg != '\0')
        {
          type = CFLAGS_OTHER;
          newarg = arg;
        }
      else
        {
          continue;
        }

      flag = flag_create (type, newarg);
      tail_list_add (&pkg->cflags, flag);
    }

  return FALSE;
}

/* Strip out -I flags, put them in a separate list. */
gboolean
cflags_parse (Package *pkg, Package *config, const char *str, const char *path)
{
  char *trimmed;
  gchar **argv = NULL;
  gint argc = 0;
  GError *error = NULL;
  gboolean die;

  if ( pkg->cflags.items != NULL )
    {
      verbose_error ("Cflags field occurs twice in '%s'\n", path);

      return parse_strict;
    }

  trimmed = package_trim_and_sub (pkg, config, str, path);
  if (trimmed == NULL)
    return TRUE;  /* Let's die */

  if ( *trimmed != '\0' && !g_shell_parse_argv (trimmed, &argc, &argv, &error) )
    {
      verbose_error ("Couldn't parse Cflags field into an argument vector: %s\n",
                     error ? error->message : "unknown");

      g_clear_error (&error);

      die = parse_strict;
      goto quit;
    }

  die = cflags_do_parse (pkg, argc, argv);

quit:

  g_free (trimmed);
  g_strfreev (argv);

  return die;
}
