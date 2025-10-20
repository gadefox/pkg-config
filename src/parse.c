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

#include <ctype.h> 
#include <stdio.h> 

#include "parse.h"
#include "cflags.h"
#include "libs.h"
#include "package.h"
#include "reqver.h"
#include "strutil.h"
#include "utils.h"


gboolean parse_strict = TRUE;
gboolean define_prefix = ENABLE_DEFINE_PREFIX;
char *prefix_variable = "prefix";

#ifdef G_OS_WIN32
gboolean msvc_syntax = FALSE;
#endif


/* A module list is a list of modules with optional version specification,
 * separated by commas and/or spaces. Commas are treated just like whitespace,
 * in order to allow stuff like: Requires: @FRIBIDI_PC@, glib, gmodule
 * where @FRIBIDI_PC@ gets substituted to nothing or to 'fribidi'
 */

typedef enum
{
  /* put numbers to help interpret lame debug spew ;-) */
  OUTSIDE_MODULE,
  IN_MODULE_NAME,
  BEFORE_OPERATOR,
  IN_OPERATOR,
  AFTER_OPERATOR,
  IN_MODULE_VERSION
} ModuleSplitState;


/*
 * Code
 */

/* ATTN: Returns FALSE when succeded; TRUE means die */
static gboolean
parse_name (Package *pkg, Package *config, const char *str, const char *path)
{
  if (pkg->name != NULL)
    {
      verbose_error ("Name field occurs twice in '%s'\n", path);
      return parse_strict;
    }

  pkg->name = package_trim_and_sub (pkg, config, str, path);
  return pkg->name == NULL;
}

/* ATTN: Returns FALSE when succeded; TRUE means die */
static gboolean
parse_version (Package *pkg, Package *config, const char *str, const char *path)
{
  if (pkg->version)
    {
      verbose_error ("Version field occurs twice in '%s'\n", path);
      return parse_strict;
    }

  pkg->version = package_trim_and_sub (pkg, config, str, path);
  return pkg->version == NULL;
}

/* ATTN: Returns FALSE when succeded; TRUE means die */
static gboolean
parse_description (Package *pkg, Package *config, const char *str, const char *path)
{
  if (pkg->description)
    {
      verbose_error ("Description field occurs twice in '%s'\n", path);
      return parse_strict;
    }

  pkg->description = package_trim_and_sub (pkg, config, str, path);
  return pkg->description == NULL;
}

static void
add_module (TailList *list, const char *start, const char *end)
{
  char *module;

  /* We left a module */
  module = g_strndup (start, end - start);

#if HAVE_PARSE_SPEW
  parse_spew ("Found module: '%s'\n", module);
#endif

  tail_list_add (list, module);
}

/* List of duplicated strings */
static GList *
split_module_list (const char *str, const char *path)
{
  TailList retval;
  const char *end;
  const char *start;
  ModuleSplitState state = OUTSIDE_MODULE;
  ModuleSplitState last_state = OUTSIDE_MODULE;
  char *temp;
  char c;

#if HAVE_PARSE_SPEW
  parse_spew ( "Parsing: '%s'\n", str);
#endif

  tail_list_init (retval);
  start = end = str;

  for ( c = *end; c != '\0'; c = *++end, last_state = state )
    {
#if HAVE_PARSE_SPEW
      parse_spew ("  char>%c [state: %d last state: %d]\n", c, state, last_state);
#endif

      switch (state)
        {
        case OUTSIDE_MODULE:

          if (!MODULE_SEPARATOR (c))
            state = IN_MODULE_NAME;

          break;

        case IN_MODULE_NAME:

          if ( IS_SPACE (c) )
            {
              /* Need to look ahead to determine next state; +1 because we know
               * the character is a space so we can move to the next character */
              temp = s_space (end + 1);

              c = *temp;
              if (c == '\0')
                state = OUTSIDE_MODULE;
              else if (MODULE_SEPARATOR (c))
                state = OUTSIDE_MODULE;
              else if (OPERATOR_CHAR (c))
                state = BEFORE_OPERATOR;
              else
                state = OUTSIDE_MODULE;
            }
          else if (MODULE_SEPARATOR (c))
            state = OUTSIDE_MODULE; /* comma precludes any operators */

          break;

        case BEFORE_OPERATOR:

          /* We know an operator is coming up here due to lookahead from
           * IN_MODULE_NAME
           */
          if (IS_SPACE (c))
            ; /* no change */
          else if (OPERATOR_CHAR (c))
            state = IN_OPERATOR;
          else
            g_assert_not_reached ();

          break;

        case IN_OPERATOR:

          if (!OPERATOR_CHAR (c))
            state = AFTER_OPERATOR;

          break;

        case AFTER_OPERATOR:

          if (!IS_SPACE (c))
            state = IN_MODULE_VERSION;

          break;

        case IN_MODULE_VERSION:

          if (MODULE_SEPARATOR (c))
            state = OUTSIDE_MODULE;

          break;

        default:

          g_assert_not_reached ();
        }

      if (state != OUTSIDE_MODULE || last_state == OUTSIDE_MODULE)
        continue;

      /* We left a module */
      add_module (&retval, start, end);

      /* reset start */
      start = end;
    }

  /* Get the last module */
  if ( start != end )
    {
      add_module (&retval, start, end);
    }

  return retval.items;
}

/* Returns RequiredVersion item list; ATTN: die is set to FALSE when succeded */
GList *
parse_module_list (Package *pkg, Package *config, const char *str, const char *path, gboolean *die)
{
  GList *split;
  GList *iter;
  TailList retval;
  RequiredVersion *ver;

  tail_list_init (retval);
  split = split_module_list (str, path);

  for ( iter = split; iter != NULL; iter = iter->next )
    {
      ver = required_version_create (pkg);

      /* ATTN: The result value does not mean success, but that the application should die*/
      if ( required_version_parse (ver, iter->data, path) )
        {
          required_version_free (ver);
          required_version_free_list (retval.items);
          free_list (split);

          *die = TRUE;
          return NULL;
        }

      tail_list_add (&retval, ver);
    }

  free_list (split);

  *die = FALSE;
  return retval.items;
}

/* ATTN: Returns FALSE when succeded; TRUE means die */
static gboolean
parse_requires (Package *pkg, Package *config, const char *str, const char *path)
{
  char *trimmed;
  gboolean die;

  if ( pkg->requires.items != NULL )
    {
      verbose_error ("Requires field occurs twice in '%s'\n", path);
      return parse_strict;
    }

  trimmed = package_trim_and_sub (pkg, config, str, path);
  if (trimmed == NULL)
    return TRUE;

  pkg->requires_entries = parse_module_list (pkg, config, trimmed, path, &die);
  g_free (trimmed);

  return die;
}

/* ATTN: Returns FALSE when succeded; TRUE means die */
static gboolean
parse_requires_private (Package *pkg, Package *config, const char *str, const char *path)
{
  char *trimmed;
  gboolean die;

  if ( pkg->requires_private.items != NULL )
    {
      verbose_error ("Requires.private field occurs twice in '%s'\n", path);
      return parse_strict;
    }

  trimmed = package_trim_and_sub (pkg, config, str, path);
  if (trimmed == NULL)
    return TRUE;

  pkg->requires_private_entries = parse_module_list (pkg, config, trimmed, path, &die);
  g_free (trimmed);

  return die;
}

/* ATTN: Returns FALSE when succeded; TRUE means die */
static gboolean
parse_conflicts (Package *pkg, Package *config, const char *str, const char *path)
{
  char *trimmed;
  gboolean die;

  if (pkg->conflicts)
    {
      verbose_error ("Conflicts field occurs twice in '%s'\n", path);
      return parse_strict;
    }

  trimmed = package_trim_and_sub (pkg, config, str, path);
  if (trimmed == NULL)
    return TRUE;

  pkg->conflicts = parse_module_list (pkg, config, trimmed, path, &die);
  g_free (trimmed);

  return die;
}

/* ATTN: Returns FALSE when succeded; TRUE means die */
static gboolean
parse_url (Package *pkg, Package *config, const char *str, const char *path)
{
  if (pkg->url != NULL)
    {
      verbose_error ("URL field occurs twice in '%s'\n", path);
      return parse_strict;
    }

  pkg->url = package_trim_and_sub (pkg, config, str, path);
  return pkg->url == NULL;
}

static gboolean
parse_keyword (Package *pkg, Package *config, const char *path,
    const char *tag, const char *value, gboolean ignore_requires,
    gboolean ignore_private_libs, gboolean ignore_requires_private)
{
  value = s_space (value);

  if (strcmp (tag, "Name") == 0)
    return parse_name (pkg, config, value, path);

  if (strcmp (tag, "Description") == 0)
    return parse_description (pkg, config, value, path);

  if (strcmp (tag, "Version") == 0)
    return parse_version (pkg, config, value, path);

  if (strcmp (tag, "Requires.private") == 0)
    {
      if (ignore_requires_private)
        return FALSE;

      return parse_requires_private (pkg, config, value, path);
    }

  if (strcmp (tag, "Requires") == 0)
    {
      if (ignore_requires)
        return FALSE;

      return parse_requires (pkg, config, value, path);
    }

  if ( strcmp (tag, "Libs.private") == 0 )
    {
      if (ignore_private_libs)
         return FALSE;

      return libs_parse_private (pkg, config, value, path);
    }

  if (strcmp (tag, "Libs") == 0)
    return libs_parse (pkg, config, value, path);

  if (strcmp (tag, "Cflags") == 0 ||
      strcmp (tag, "CFlags") == 0)
    return cflags_parse (pkg, config, value, path);

  if (strcmp (tag, "Conflicts") == 0)
    return parse_conflicts (pkg, config, value, path);

  if (strcmp (tag, "URL") == 0)
    return parse_url (pkg, config, value, path);

  /* we don't error out on unknown keywords because they may
   * represent additions to the .pc file format from future
   * versions of pkg-config.  We do make a note of them in the
   * debug spew though, in order to help catch mistakes in .pc
   * files. */
  debug_spew ("Unknown keyword '%s' in '%s'\n", tag, path);
  return FALSE;
}

static int
parse_prefix_variable (Package *pkg, const char *tag, const char *prefix)
{
  gchar *value;
  gchar *temp;

  /* This is the prefix variable. Try to guesstimate a value for it
   * for this package from the location of the .pc file. */
  value = g_path_get_basename (pkg->pcfiledir);

  if ( g_ascii_strcasecmp (value, "pkgconfig") != 0 )
    {
      g_free (value);
      return FALSE;
	  }

  /* It ends in pkgconfig. Good. */
  g_free (value);

  /* Keep track of the original prefix value. */
  pkg->orig_prefix = g_strdup (prefix);

  /* Get grandparent directory for new prefix. */
  temp = g_path_get_dirname (pkg->pcfiledir);
  value = g_path_get_dirname (temp);
  g_free (temp);

  /* Turn backslashes into slashes or g_shell_parse_argv() will eat
   * them when ${prefix} has been expanded in libs_parse(). */
  backslash_to_slash (value);

  /* Now escape the special characters so that there's no danger
  * of arguments that include the prefix getting split. */
  temp = s_dup_escape_shell (value);
  g_free (value);

  debug_spew (" Variable declaration, '%s' overridden with '%s'\n", tag, temp);

  value = g_strdup (tag);
  g_hash_table_insert (pkg->vars, value, temp);

  return TRUE;
}

/* Oh my.. 3 times to use 'strlen' in the previous code is called an optimization */
static char *
parse_orig_prefix (Package *pkg, const char *prefix)
{
  int len;
  char *varval;
	char c;

  if (pkg->orig_prefix == NULL)
    return NULL;

  len = strlen (pkg->orig_prefix);
  if ( len == 0 )		/* ~ pkg->orig_prefix[0] == '\0' ~ *(pkg->orig_prefix) == '\0' */
    return NULL;

  if ( strncmp (pkg->orig_prefix, prefix, len) != 0 )
    return NULL;

  c = prefix [len];
  if ( !G_IS_DIR_SEPARATOR (c) )
    return NULL;

  varval = g_hash_table_lookup (pkg->vars, prefix_variable);
  return g_strconcat (varval, prefix + len, NULL);
}

static gboolean
parse_line (Package *pkg, Package *config, const char *untrimmed,
        const char *path, gboolean ignore_requires,
        gboolean ignore_private_libs, gboolean ignore_requires_private)
{
  char *str, *tag, *end;
  char *newval;
  char c;
  gboolean die;

  debug_spew ("  line>%s\n", untrimmed);

  str = s_trim (untrimmed);

  end = s_valid (str);
  if ( str == end )   /* empty line */
    {
      g_free (str);
      return FALSE;
    }

  die = FALSE;
  tag = g_strndup (str, end - str);
  end = s_space (end);

  c = *end;
  if (c == ':')
    {
      /* keyword */
      die = parse_keyword (pkg, config, path, tag, end + 1, ignore_requires,
                          ignore_private_libs, ignore_requires_private);
    }
  else if (c == '=')
    {
      end = s_space (end + 1);

      if ( define_prefix )
        {
          if ( strcmp (tag, prefix_variable) == 0)
            {
              if ( parse_prefix_variable (pkg, tag, end) )
                goto cleanup;
            }
          else
            {
              newval = parse_orig_prefix (pkg, end);
              if ( newval != NULL )
                {
                  g_free (str);
                  str = end = newval;
                }
            }
        }

      if ( g_hash_table_lookup (pkg->vars, tag) != NULL )
        {
          verbose_error ("Duplicate definition of variable '%s' in '%s'\n",
                         tag, path);

          if (parse_strict)
            {
              die = TRUE;
              goto cleanup;
            }
        }

      newval = package_trim_and_sub (pkg, config, end, path);
      if (newval == NULL)
        {
          die = TRUE;
          goto cleanup;
        }

      debug_spew (" Variable declaration, '%s' has value '%s'\n",
                  tag, newval);

      end = g_strdup (tag);
      g_hash_table_insert (pkg->vars, end, newval);
    }

cleanup:

  g_free (tag);
  g_free (str);
  return die;
}

Package*
parse_package_file (const char *key, const char *path,
                    Package *pkg_config,
                    gboolean ignore_requires,
                    gboolean ignore_private_libs,
                    gboolean ignore_requires_private,
                    gboolean *die )
{
  FILE *f;
  Package *pkg;
  GString *str;
  gboolean one_line = FALSE;

  f = fopen (path, "r");

  if (f == NULL)
    {
      verbose_error ("Failed to open '%s': %s\n", path, strerror (errno));

      /* 'parse_strict' boolean is relevant only when return value is null */
      *die = FALSE;
      return NULL;
    }

  debug_spew ("Parsing package file '%s'\n", path);

  pkg = g_new0 (Package, 1);
  pkg->key = g_strdup (key);

  if ( path != NULL )
    {
      pkg->pcfiledir = g_path_get_dirname (path);
    }
  else
    {
      debug_spew ("No pcfiledir determined for package\n");
      pkg->pcfiledir = g_strdup ("???????");
    }

  /* Variable storing directory of pc file */
  package_add_var (pkg, "pcfiledir", pkg->pcfiledir);

  str = g_string_new ("");

  while (read_one_line (f, str))
    {
      one_line = TRUE;

      if ( parse_line (pkg, pkg_config, str->str, path, ignore_requires,
            ignore_private_libs, ignore_requires_private) )
        goto quit;

      g_string_truncate (str, 0);
    }

  if (!one_line)
    verbose_error ("Package file '%s' appears to be empty\n", path);

  g_string_free (str, TRUE);
  fclose(f);

  /* No need to set 'die' boolean because package is not null */
  return pkg;

quit:

  package_free ( pkg );

  g_string_free (str, TRUE);
  fclose(f);

  *die = TRUE;
  return NULL;
}

/* Parse a package variable. When the value appears to be quoted,
 * unquote it so it can be more easily used in a shell. Otherwise,
 * return the raw value.
 */
char *
parse_package_variable (Package *pkg, Package *config, const char *variable)
{
  char *value;
  char *unquoted;
  GError *error = NULL;

  value = package_get_var_globals (pkg, config, variable);
  if ( value == NULL )
    return NULL;

  if (*value != '"' && *value != '\'')
    /* Not quoted, return raw value */
    return value;

  /* Maybe too naive, but assume a fully quoted variable */
  unquoted = g_shell_unquote (value, &error);
  if (unquoted != NULL)
    {
      g_free (value);
      return unquoted;
    }

  /* Note the issue, but just return the raw value */
  debug_spew ("Couldn't unquote value of \"%s\": %s\n",
              variable, error ? error->message : "unknown");

  g_clear_error (&error);
  return value;
}
