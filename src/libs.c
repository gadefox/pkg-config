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

#include "libs.h"
#include "flag.h"
#include "globals.h"
#include "parse.h"
#include "strutil.h"
#include "utils.h"


/*
 * Code
 */

void
lib_init_system_dirs (Package *pkg_config)
{
  const char *search_path;

  /* SystemLibraryPath */
  search_path = getenv ("PKG_CONFIG_SYSTEM_LIBRARY_PATH");
  if (search_path == NULL)
    {
      search_path = package_get_var (pkg_config, "system_library_path");
      if (search_path == NULL)
        search_path = PKG_CONFIG_SYSTEM_LIBRARY_PATH;
    }

  lib_add_system_dirs (search_path);
}

void
libs_verify (Package *pkg, Package *config)
{
  GList *iter;
  char *libs;
  Flag *flag;

  for ( iter = pkg->libs.items; iter != NULL; )
    {
      flag = iter->data;

      if ( !(flag->type & LIBS_L) )
        {
          /* Move to the next item in the list */
          iter = iter->next;
          continue;
        }

      /* Check '-L' first. */
      libs = flag->arg;
      if ( *libs++ != '-' ||  /* An empty string or doesn't begin w/ '-' character */
           *libs++ != 'L' )   /* "-" string or doesn't begin w/ "-L" */
        {
          /* Move to the next item in the list */
          iter = iter->next;
          continue;
        }

      /* Check the space then */
      if ( *libs == ' ' )
        libs++;

      if ( !lib_is_system_dirs (libs, pkg->key, flag->arg ) )
        {
          /* Move to the next item in the list */
          iter = iter->next;
          continue;
        }

      debug_spew ("Removing %s from Libs for %s\n", flag->arg, pkg->key);

      /* Remove the current item from the list including freeing the memory. Please note the
       * pointer to the list is changed when it's the first item in the list. Then we move to
       * the next item. */
      tail_list_remove (&pkg->libs, iter);

#if !GLIB_CHECK_VERSION(2,28,0)
      /* Don't forget to destroy flag object */
      free_flag (flag);
#endif
    }
}

gboolean
lib_is_system_dirs (const char* compare, const char *key, const char *arg)
{
  GList *iter;

  for ( iter = lib_system_dirs.items; iter != NULL; iter = iter->next )
    {
      if (strcmp (iter->data, compare) != 0)
        continue;

      debug_spew ("Package %s has %s in Libs\n", key, arg);

      if (allow_system_libs)
        continue;

      return TRUE;
    }

  return FALSE;
}

void
lib_add_system_dirs (const gchar *dirs)
{
  gchar **values;
  gchar **iter;
  gchar *val;

  values = g_strsplit (dirs, G_SEARCHPATH_SEPARATOR_S, 0);

  for ( iter = values, val = *iter; val != NULL; val = *++iter)
    {
      val = g_strdup (val);
      tail_list_add ( &lib_system_dirs, val);
    }

  g_strfreev (values);
}

/* ATTN: Returns FALSE when succeded; TRUE means die */
static gboolean
libs_do_parse (Package *pkg, int argc, char **argv)
{
#ifdef G_OS_WIN32
  char *L_flag = (msvc_syntax ? "/libpath:" : "-L");
  char *l_flag = (msvc_syntax ? "" : "-l");
  char *lib_suffix = (msvc_syntax ? ".lib" : "");
#else
  char *L_flag = "-L";
  char *l_flag = "-l";
  char *lib_suffix = "";
#endif // G_OS_WIN32

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
           p[1] == 'l' &&
          /* -lib: is used by the C# compiler for libs; it's not an -l flag. */
           strncmp(p + 2, "ib:", 3) != 0 )  /* ~ strncmp(p, "-lib:", 5) != 0 */
        {
          p = s_space (p + 2); /* 2 because "-l" */

          type = LIBS_l;
          newarg = g_strconcat (l_flag, p, lib_suffix, NULL);
          g_free (arg);
        }
      else if (p[0] == '-' &&
               p[1] == 'L')
        {
          p = s_space (p + 2); /* 2 because "-L" */

          type = LIBS_L;
          newarg = g_strconcat (L_flag, p, NULL);
          g_free (arg);
        }
      else if ( (strcmp("-framework", p) == 0 ||
                 strcmp("-Wl,-framework", p) == 0) &&
                 i + 1 < argc )
        {
          /* Mac OS X has a -framework Foo which is really one option,
           * so we join those to avoid having -framework Foo
           * -framework Bar being changed into -framework Foo Bar
           * later
          */
          tmp = s_trim (argv[++i]);
          if (tmp == NULL)
            {
              g_free (arg);
              return TRUE;
            }

          p = s_dup_escape_shell (tmp);
          g_free (tmp);

          type = LIBS_OTHER;
          newarg = g_strconcat (arg, " ", p, NULL);
          g_free (p);
          g_free (arg);
        }
      else if (*arg != '\0')
        {
          type = LIBS_OTHER;
          newarg = arg;
        }
      else
        {
          continue;
        }

      flag = flag_create (type, newarg);
      tail_list_add (&pkg->libs, flag);
    }

  return FALSE;
}

/* Strip out -l and -L flags, put them in a separate list.
 * ATTN: Returns FALSE when succeded; TRUE means die */
gboolean
libs_parse (Package *pkg, Package *config, const char *str, const char *path)
{
  char *trimmed;
  gchar **argv = NULL;
  gint argc = 0;
  GError *error = NULL;
  gboolean die;

  if (pkg->libs_num > 0)
    {
      verbose_error ("Libs field occurs twice in '%s'\n", path);
      return parse_strict;
    }

  trimmed = package_trim_and_sub (pkg, config, str, path);
  if ( trimmed == NULL )
    return TRUE;

  /* g_shell_parse_argv returns FALSE when the parsing text is empty */
  if ( *trimmed != '\0' && !g_shell_parse_argv (trimmed, &argc, &argv, &error) )
    {
      verbose_error ("Couldn't parse Cflags field into an argument vector: %s\n",
                     error ? error->message : "unknown");

      g_clear_error (&error);

      die = parse_strict;
      goto quit;
    }

  die = libs_do_parse (pkg, argc, argv);
  if ( !die )
    pkg->libs_num++;

quit:

  g_strfreev (argv);
  g_free (trimmed);
  return die;
}

/*
 * List of private libraries.  Private libraries are libraries which
 * are needed in the case of static linking or on platforms not
 *  * supporting inter-library dependencies.  They are not supposed to
 * be used for libraries which are exposed through the library in
 * question.  An example of an exposed library is GTK+ exposing Glib.
 * A common example of a private library is libm.

 * Generally, if include another library's headers in your own, it's
 * a public dependency and not a private one.

 * ATTN: Returns FALSE when succeded; TRUE means die */
gboolean
libs_parse_private (Package *pkg, Package *config, const char *str, const char *path)
{
  char *trimmed;
  gchar **argv = NULL;
  gint argc = 0;
  GError *error = NULL;
  gboolean die;

  if (pkg->libs_private_num > 0)
    {
      verbose_error ("Libs.private field occurs twice in '%s'\n", path);
      return parse_strict;
    }

  trimmed = package_trim_and_sub (pkg, config, str, path);

  if ( trimmed == NULL )
    return TRUE;    /* Let's die */

  /* g_shell_parse_argv returns FALSE when the parsing text is empty */
  if ( *trimmed != '\0' && !g_shell_parse_argv (trimmed, &argc, &argv, &error) )
    {
      verbose_error ("Couldn't parse Libs.private field into an argument vector: %s\n",
                     error ? error->message : "unknown");

      g_clear_error (&error);

      die = parse_strict;
      goto quit;
    }

  die = libs_do_parse (pkg, argc, argv);
  if ( !die )
    pkg->libs_private_num++;

quit:

  g_free (trimmed);
  g_strfreev (argv);
  return die;
}
