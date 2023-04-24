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

#include "reqver.h"
#include "globals.h"
#include "strutil.h"
#include "utils.h"


/*
 * Code
 */

void
required_version_free (RequiredVersion *rv)
{
  g_free (rv->name);
  g_free (rv->version);
  g_free (rv);
}

RequiredVersion *
required_version_create (Package *owner)
{
  RequiredVersion *ver;

  ver = g_new0 (RequiredVersion, 1);
  ver->owner = owner;
  return ver;
}

#if !GLIB_CHECK_VERSION(2,28,0)

static void
required_version_free_list_foreach (gpointer data, gpointer user_data)
{
  required_version_free (data);
}

void
required_version_free_list (GList *list)
{
  g_list_foreach (list, required_version_free_list_foreach, NULL);
  g_list_free (list);
}

#endif  /* GLIB_CHECK_VERSION */

const char *
comparison_to_str (ComparisonType comparison)
{
  switch (comparison)
    {
    case LESS_THAN:
      return "<";

    case GREATER_THAN:
      return ">";

    case LESS_THAN_EQUAL:
      return "<=";

    case GREATER_THAN_EQUAL:
      return ">=";

    case EQUAL:
      return "=";

    case NOT_EQUAL:
      return "!=";

    case ALWAYS_MATCH:
      return "(any)";

    case UNKNOWN:
      break;
    }

  return "(unknown)";
}

gboolean
version_test (ComparisonType comparison,
              const char *a,
              const char *b)
{
  switch (comparison)
    {
    case LESS_THAN:
      return compare_versions (a, b) < 0;

    case GREATER_THAN:
      return compare_versions (a, b) > 0;

    case LESS_THAN_EQUAL:
      return compare_versions (a, b) <= 0;

    case GREATER_THAN_EQUAL:
      return compare_versions (a, b) >= 0;

    case EQUAL:
      return compare_versions (a, b) == 0;

    case NOT_EQUAL:
      return compare_versions (a, b) != 0;

    case ALWAYS_MATCH:
      return TRUE;

    case UNKNOWN:
      return FALSE;
    }

  g_assert_not_reached ();
  return FALSE;
}

void
required_versions_add (Package *pkg, RequiredVersion *ver)
{
  if (pkg->required_versions == NULL)
    pkg->required_versions = g_hash_table_new (g_str_hash, g_str_equal);

  g_hash_table_insert (pkg->required_versions, ver->name, ver);
}

/* ATTN: Returns FALSE when succeded; TRUE means die */
gboolean
required_version_parse (RequiredVersion *ver, const char *value, const char* path)
{
  char *start, *end;

  end = s_module_sep (value);

  /* name */
  start = end;
  end = s_not_space (end);
  end = scut_module_sep (end);

  if (*start == '\0')
    {
      verbose_error ("Empty package name in Requires or Conflicts in file '%s'\n", path);
      return parse_strict;
    }

  ver->name = g_strdup (start);

  /* comparator */
  start = end;
  end = s_not_space (end);
  end = scut_space (end);

  ver->comparison = parse_comparison_type (start);
  if ( ver->comparison == UNKNOWN )
    {
      verbose_error ("Unknown version comparison operator '%s' after "
                     "package name '%s' in file '%s'\n", start,
                     ver->name, path);

      return parse_strict;
    }

  /* comparison */
  start = end;
  end = s_not_module_sep (end);
  end = scut_module_sep (end);

  if ( *start == '\0' )
    {
      if ( ver->comparison != ALWAYS_MATCH )
        {
          verbose_error ("Comparison operator but no version after package "
                         "name '%s' in file '%s'\n", ver->name, path);

          if ( parse_strict )
            return TRUE;

          ver->version = g_strdup ("0");
          return FALSE;
        }
    }
  else
    {
      ver->version = g_strdup (start);
    }

  g_assert (ver->name != NULL );

  return FALSE;
}

ComparisonType
parse_comparison_type (const char *value)
{
  char c;

  c = *value;

  /* empty string? */
  if (c == '\0')
    return ALWAYS_MATCH;

  /* '=' */
  if (c == '=')
    {
      c = *++value;
      return c == '\0' ? EQUAL : UNKNOWN;
    }

  /* '>' or '>=' */
  if (c == '>')
    {
      /* '>' */
      c = *++value;
      if (c == '\0')
        return GREATER_THAN;

      /* '>=' */
      if (c != '=')
        return UNKNOWN;

      c = *++value;
      return c == '\0' ? GREATER_THAN_EQUAL : UNKNOWN;
    }

  /* '<' or '<=' */
  if (c == '<')
    {
      /* '<' */
      c = *++value;
      if (c == '\0')
        return LESS_THAN;

      /* '<=' */
      if (c != '=')
        return UNKNOWN;

      c = *++value;
      return c == '\0' ? LESS_THAN_EQUAL : UNKNOWN;
    }

  /* '!=' */
  if (c == '!')
    {
      c = *++value;
      if (c != '=')
        return UNKNOWN;

      c = *++value;
      return c == '\0' ? NOT_EQUAL : UNKNOWN;
    }

  return UNKNOWN;
}
