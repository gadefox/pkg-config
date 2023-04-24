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

#include "flag.h"
#include "globals.h"
#include "package.h"
#include "utils.h"


/*
 * Code
 */

void
flag_free (Flag *flag)
{
  g_free (flag->arg);
  g_free (flag);
}

Flag *
flag_create (FlagType type, char *arg)
{
  Flag *flag;

  flag = g_new0 (Flag, 1);
  flag->type = type;
  flag->arg = arg;
  return flag;
}

#if !GLIB_CHECK_VERSION(2,28,0)

static void
flag_list_foreach_free (gpointer data, gpointer user_data)
{
  flag_free (data);
}

void
flag_free_list (GList *list)
{
  g_list_foreach (list, flag_list_foreach_free, NULL);
  g_list_free (list);
}

#endif // !GLIB_CHECK_VERSION

char *
flags_packages_get (GList *pkgs, FlagType flags)
{
  GString *str;
  char *cur;

  str = g_string_new (NULL);

  /* sort packages in path order for -L/-I, dependency order otherwise */
  if (flags & CFLAGS_OTHER)
    {
      cur = flag_get_multi_merged (pkgs, CFLAGS_OTHER, FALSE, TRUE);
      debug_spew ("adding CFLAGS_OTHER string \"%s\"\n", cur);
      g_string_append (str, cur);
      g_free (cur);
    }

  if (flags & CFLAGS_I)
    {
      cur = flag_get_multi_merged (pkgs, CFLAGS_I, TRUE, TRUE);
      debug_spew ("adding CFLAGS_I string \"%s\"\n", cur);
      g_string_append (str, cur);
      g_free (cur);
    }

  if (flags & LIBS_L)
    {
      cur = flag_get_multi_merged (pkgs, LIBS_L, TRUE, !ignore_private_libs);
      debug_spew ("adding LIBS_L string \"%s\"\n", cur);
      g_string_append (str, cur);
      g_free (cur);
    }

  if (flags & (LIBS_OTHER | LIBS_l))
    {
      cur = flag_get_multi_merged (pkgs, flags & (LIBS_OTHER | LIBS_l), FALSE,
                              !ignore_private_libs);

      debug_spew ("adding LIBS_OTHER | LIBS_l string \"%s\"\n", cur);
      g_string_append (str, cur);
      g_free (cur);
    }

  /* Strip trailing space. */
  if (str->len > 0 && str->str[str->len - 1] == ' ')
    g_string_truncate (str, str->len - 1);

  debug_spew ("returning flags string \"%s\"\n", str->str);
  return g_string_free (str, FALSE);
}

/* Merge the flags from the individual packages
 * List of flag pointers */
GList *
flag_merge_lists (GList *packages, FlagType type)
{
  TailList merged;
  Package *pkg;
  GList *flags;
  Flag *flag;

  tail_list_init (merged);

  /* Keep track of the last element to avoid traversing the whole list */
  for ( ; packages != NULL; packages = packages->next )
    {
      pkg = packages->data;

      /* Manually copy the elements so we can keep track of the end */
      for ( flags = (type & LIBS_ANY) ? pkg->libs.items : pkg->cflags.items; flags != NULL; flags = flags->next )
        {
          flag = flags->data;

          if ( flag->type & type )
            tail_list_add (&merged, flag);
        }
    }

  return merged.items;
}

char *
flag_list_to_string (GList *list)
{
  GList *iter;
  GString *str;
  char *retval;
  Flag *flag;
  char *arg;
  char *space;

  str = g_string_new ("");

  for ( iter = list; iter != NULL; iter = iter->next )
    {
      flag = iter->data;
      arg = flag->arg;

      if (pcsysrootdir != NULL && flag->type & (CFLAGS_I | LIBS_L))
        {
          /* Handle non-I Cflags like -isystem.. [strncmp (arg, "-I", 2)
           * == 0] ~ [arg[0] == '-' && arg[1] == 'I']
           */
          if (flag->type & CFLAGS_I && (arg[0] != '-' || arg[1] != 'I'))
            {
              space = strchr (arg, ' ');

              /* Ensure this has a separate arg */
              g_assert (space != NULL && space[1] != '\0');

              space++;
              g_string_append_len (str, arg, space - arg);
              g_string_append (str, pcsysrootdir);
              g_string_append (str, space);
            }
          else
            {
              g_string_append_c (str, '-');
              g_string_append_c (str, arg[1]);
              g_string_append (str, pcsysrootdir);
              g_string_append (str, arg + 2);
            }
        }
      else
        {
          g_string_append (str, arg);
        }

      g_string_append_c (str, ' ');
    }

  retval = str->str;
  g_string_free (str, FALSE);

  return retval;
}

/* Strip consecutive duplicate arguments in the flag list. */
GList *
flag_list_strip_duplicates (GList *list)
{
  GList *iter;
  Flag *curr_flag;
  Flag *prev_flag;
  GList *temp;

  if (list == NULL)
    return NULL;

  /* Start at the 2nd element of the list so we don't have to check for an
   * existing previous element. */
  for ( prev_flag = list->data, iter = list->next; iter != NULL; /* Nop! */ )
    {
      curr_flag = iter->data;

      if (curr_flag->type != prev_flag->type || strcmp (curr_flag->arg, prev_flag->arg) != 0)
        {
          prev_flag = curr_flag;
          iter = iter->next;
          continue;
        }

      debug_spew (" removing duplicate \"%s\"\n", curr_flag->arg);

      /* Remove the duplicate flag object from the list and move to the next item */
      temp = iter;
      iter = iter->next;
      list = g_list_delete_link (list, temp);

#if !GLIB_CHECK_VERSION(2,28,0)
      free_flag (curr_flag);
#endif
    }

  return list;
}

/* Create a merged list of required packages and retrieve the flags from them.
 * Strip the duplicates from the flags list. The sorting and stripping can be
 * done in one of two ways: packages sorted by position in the pkg-config path
 * and stripping done from the beginning of the list, or packages sorted from
 * most dependent to least dependent and stripping from the end of the list.
 * The former is done for -I/-L flags, and the latter for all others.
 */
char *
flag_get_multi_merged (GList *pkgs, FlagType type, gboolean in_path_order,
                  gboolean include_private)
{
  GList *list;
  char *retval;

  list = fill_list (pkgs, type, in_path_order, include_private);
  list = flag_list_strip_duplicates (list);
  retval = flag_list_to_string (list);
  g_list_free (list);

  return retval;
}


