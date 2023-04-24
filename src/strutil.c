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
#include <string.h>

#include "strutil.h"
#include "parse.h"



#define UNINSTALLED_LEN  (sizeof ("-uninstalled") - 1)

#ifdef G_OS_WIN32
 /* Guard against .pc file being installed with UPPER CASE name */
 #define FOLD(x) tolower(x)
 #define FOLDCMP(a, b)  g_ascii_strcasecmp (a, b)
#else
 #define FOLD(x) (x)
 #define FOLDCMP(a, b)  strcmp (a, b)
#endif

/* macros to help code look more like upstream */
#define rstreq(a, b)  (strcmp (a, b) == 0)
#define risalnum(c)	 isalnum ((guchar) (c))
#define risdigit(c)	 isdigit ((guchar) (c))
#define risalpha(c)	 isalpha ((guchar) (c))


/*
 * Code
 */

/* compare alpha and numeric segments of two versions */
/* return 1: a is newer than b */
/*        0: a and b are the same version */
/*       -1: b is newer than a */
int
compare_versions (const char *a, const char *b)
{
    char oldch1, oldch2;
    char *str1, *str2;
    char *one, *two;
    int rc;
    int isnum, lenone, lentwo;
    char cone, ctwo;

    /* easy comparison to see if versions are identical */
    if (rstreq (a, b))
       return 0;

    str1 = g_alloca (strlen (a) + 1);
    str2 = g_alloca (strlen (b) + 1);

    strcpy (str1, a);
    strcpy (str2, b);

    one = str1;
    two = str2;

    /* loop through each version segment of str1 and str2 and compare them */
    for ( cone = *one, ctwo = *two; cone != '\0' && ctwo != '\0'; cone = *one, ctwo = *two ) {
      for ( ; cone != '\0' && !risalnum (cone); cone = *++one )
        ;  /* NOP */
      for ( ; ctwo != '\0' && !risalnum (ctwo); ctwo = *++two )
        ;  /* NOP */

    	/* If we ran to the end of either, we are finished with the loop */
	    if ( cone == '\0' || ctwo == '\0' )
        break;

    	str1 = one;
        cone = *str1;
	    str2 = two;
        ctwo = *str2;

    	/* grab first completely alpha or completely numeric segment */
	    /* leave one and two pointing to the start of the alpha or numeric */
    	/* segment and walk str1 and str2 to end of segment */
	    if ( risdigit (cone) ) {
            for ( ; cone != '\0' && risalnum (cone); cone = *++str1 )
                ;  /* NOP */
            for ( ; ctwo != '\0' && risalnum (ctwo); ctwo = *++str2 )
                ;  /* NOP */
    	    isnum = TRUE;
    	} else {
	        for ( ; cone != '\0' && risalpha (cone); cone = *++str1 )
                ;  /* NOP */
            for ( ; ctwo != '\0' && risalpha (ctwo); ctwo = *++str2 )
                ;  /* NOP */
    	    isnum = FALSE;
	    }

    	/* save character at the end of the alpha or numeric segment */
	    /* so that they can be restored after the comparison */
    	oldch1 = cone;
	    *str1 = '\0';
    	oldch2 = ctwo;
	    *str2 = '\0';

    	/* this cannot happen, as we previously tested to make sure that */
	    /* the first string has a non-null segment */
    	if ( one == str1 )
            return -1;	/* arbitrary */

    	/* take care of the case where the two version segments are */
	    /* different types: one numeric, the other alpha (i.e. empty) */
    	/* numeric segments are always newer than alpha segments */
	    /* XXX See patch #60884 (and details) from bugzilla #50977. */
    	if ( two == str2 )
            return isnum ? 1 : -1;

    	if (isnum) {
    	    /* this used to be done by converting the digit segments */
	        /* to ints using atoi() - it's changed because long  */
	        /* digit segments can overflow an int - this should fix that. */

    	    /* throw away any leading zeros - it's a number, right? */
	        while ( *one == '0' )
               one++;
	        while ( *two == '0' )
               two++;

    	    /* whichever number has more digits wins */
            lenone = strlen (one);
            lentwo = strlen (two);
	        if ( lenone > lentwo )
               return 1;

    	    if ( lentwo > lenone )
               return -1;
    	}

    	/* strcmp will return which one is greater - even if the two */
	    /* segments are alpha or if they are numeric.  don't return  */
    	/* if they are equal because there might be more segments to */
	    /* compare */
    	rc = strcmp (one, two);
    	if ( rc != 0 )
            return rc < 1 ? -1 : 1;

    	/* restore character that was replaced by null above */
    	*str1 = oldch1;
	    one = str1;
    	*str2 = oldch2;
	    two = str2;
    }

    /* this catches the case where all numeric and alpha segments have */
    /* compared identically but the segment sepparating characters were */
    /* different */
    if ( *one == '\0' ) {
        if ( *two == '\0' )
            return 0;

        /* whichever version still has characters left over wins */
        return -1;
    }
    return 1;
}

char *
var_to_pkg_config_var (const char *key, const char *var)
{
  char *p;
  char c;
  char *new;

  if ( key != NULL )
    new = g_strconcat (key, ".", var, NULL);
  else
    new = g_strdup (var);

  for (p = new, c = *p; c != '\0'; c = *++p)
    {
      if (g_ascii_isalnum (c))
        c = g_ascii_tolower (c);
      else
        c = '_';

      *p = c;
    }

  return new;
}

char *
var_to_env_var (const char *key, const char *var)
{
  char *p;
  char c;
  char *new;

  new = g_strconcat ("PKG_CONFIG_", key, "_", var, NULL);

  for (p = new, c = *p; c != '\0'; c = *++p)
    {
      if (g_ascii_isalnum (c))
        c = g_ascii_toupper (c);
      else
        c = '_';

      *p = c;
    }
  return new;
}

gboolean
is_str_one_text (const char *value)
{
  /* All conditions include '\0' so we don't need to check the length etc. */
  if ( *value++ != '1' ) return FALSE;

  return *value == '\0';
}

gboolean
is_str_true_text (const char *value)
{
  /* All conditions include '\0' so we don't need to check the length etc. */
  if ( *value++ != 't' ) return FALSE;
  if ( *value++ != 'r' ) return FALSE;
  if ( *value++ != 'u' ) return FALSE;
  if ( *value++ != 'e' ) return FALSE;

  return *value == '\0';
}

gboolean
name_ends_in_uninstalled (const char *str)
{
  int len = strlen (str);

  if (len > UNINSTALLED_LEN &&
      FOLDCMP ((str + len - UNINSTALLED_LEN), "-uninstalled") == 0)
    return TRUE;

  return FALSE;
}

gboolean
ends_in_dotpc (const char *str)
{
  int len = strlen (str);

  if (len > EXT_LEN &&
      str[len - 3] == '.' &&
      FOLD (str[len - 2]) == 'p' &&
      FOLD (str[len - 1]) == 'c')
    return TRUE;

  return FALSE;
}

char *
s_dup_escape_shell (const char *str)
{
  size_t len;
  char *val, *p;
  char c;
  unsigned int used;

  len = strlen (str) + 10;
  p = val = g_malloc (len);
  used = 0;

  for ( c = *str; c != '\0' ; c = *++str )
  {
    if ((c < '$') ||
        (c > '$' && c < '(') ||
        (c > ')' && c < '+') ||
        (c > ':' && c < '=') ||
        (c > '=' && c < '@') ||
        (c > 'Z' && c < '^') ||
        (c == '`') ||
        (c > 'z' && c < '~') ||
        (c > '~'))
      {
        *p++ = '\\';
        used++;
      }

    *p++ = c;
    used++;

    /* 3 because one character for '\0' + 2 possible characters
     * in next iteration */
    if ( used + 3 >= len )
      {
        /* Double the buffer */
        len *= 2;

        /* Reallocate the memory and don't forget to set 'p' pointer */
        val = g_realloc(val, len);
        p = val + used;
      }
  }

  *p = '\0';
  return val;
}

/**
 * Read an entire line from a file into a buffer. Lines may
 * be delimited with '\n', '\r', '\n\r', or '\r\n'. The delimiter
 * is not written into the buffer. Text after a '#' character is treated as
 * a comment and skipped. '\' can be used to escape a # character.
 * '\' proceding a line delimiter combines adjacent lines. A '\' proceding
 * any other character is ignored and written into the output buffer
 * unmodified.
 *
 * Return value: %FALSE if the stream was already at an EOF character.
 **/
gboolean
read_one_line (FILE *stream, GString *str)
{
  gboolean quoted = FALSE;
  gboolean comment = FALSE;
  int n_read = 0;
  int c;
  int next_c;

  g_string_truncate (str, 0);

  while (TRUE)
    {
      c = getc (stream);

      if (c == EOF)
        {
          if (quoted)
            g_string_append_c (str, '\\');

          goto done;
        }

      n_read++;

      if (quoted)
        {
          quoted = FALSE;
          switch (c)
          {
            case '#':
              g_string_append_c (str, '#');
              break;

            case '\r':
            case '\n':
                next_c = getc (stream);
                if (c != EOF &&
                   (c != '\r' || next_c != '\n') &&
                   (c != '\n' || next_c != '\r'))
                  ungetc (next_c, stream);
                break;

            default:
              g_string_append_c (str, '\\');
              g_string_append_c (str, c);
              break;
          }
        }
      else
        {
          switch (c)
          {
            case '#':
              comment = TRUE;
              break;

            case '\\':
              if (!comment)
                quoted = TRUE;
              break;

            case '\n':
                next_c = getc (stream);
                if (c != EOF &&
                   (c != '\r' || next_c != '\n') &&
                   (c != '\n' || next_c != '\r'))
                  ungetc (next_c, stream);
                goto done;

            default:
              if (!comment)
                g_string_append_c (str, c);
              break;
          }
        }
    }

done:
  return n_read > 0;
}

void
backslash_to_slash (char *p)
{
  char c;

  for ( c = *p; c != '\0'; c = *++p )
    {
      if (c == '\\')
        *p = '/';
    }
}

char *
s_valid (const char *p)
{
  char c;

  /* Get first word */
  for ( c = *p; IS_VALID_CHAR (c); c = *++p )
    ; /* Nop */

  return (char*)p;
}

char *
s_module_sep (const char *p)
{
  char c;

  for ( c = *p; c != '\0' && MODULE_SEPARATOR (c); c = *++p )
    ; /* Nop */

  return (char*)p;
}

char *
s_not_module_sep (const char *p)
{
  char c;

  for ( c = *p; c != '\0' && !MODULE_SEPARATOR (c); c = *++p )
    ; /* Nop */

  return (char*)p;
}

char *
scut_module_sep (char *p)
{
  char c;

  for ( c = *p; c != '\0' && MODULE_SEPARATOR (c); c = *++p )
    *p = '\0';

  return p;
}

char *
s_space (const char *p)
{
  char c;

  for ( c = *p; c != '\0' && IS_SPACE (c); c = *++p )
    ; /* Nop */

  return (char*)p;
}

char *
s_space_equal (const char *p)
{
  char c;

  for ( c = *p; c != '\0' && c != '=' && c != ' '; c = *++p )
    ; /* Nop */

  return (char*)p;
}

char *
scut_not_space_equal (char *p)
{
  char c;

  for ( c = *p; c != '\0' && (c == '=' || c == ' '); c = *++p )
    *p = '\0';

  return (char*)p;
}

char *
s_end_bracket (const char *p)
{
  char c;

  for ( c = *p; c != '\0' && c != '}'; c = *++p )
    ; /* Nop */

  return (char*)p;
}

char *
s_not_space (const char *p)
{
  char c;

  for ( c = *p; c != '\0' && !IS_SPACE (c); c = *++p )
    ; /* Nop */

  return (char*)p;
}

char *
scut_space (char *p)
{
  char c;

  for ( c = *p; c != '\0' && IS_SPACE (c); c = *++p )
    *p = '\0';

  return p;
}

char *
s_trim (const char *str)
{
  unsigned int len;
  char *end;
  char c;

  g_return_val_if_fail (str != NULL, NULL);

  str = s_space (str);
  len = strlen (str);

  /* An empty string is trimed by a definition. When the string consists of only
   * one character we know it's not a space because we called str_space function
   * already */
  if ( len > 1 )
    {
      end = (char*)str + len - 1;

	  for ( c = *end; len != 0 && IS_SPACE (c); c = *--end )
	    len--;
    }
  return g_strndup (str, len);
}
