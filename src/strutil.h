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

#ifndef _STRUTIL_H_
#define _STRUTIL_H_

#include <glib.h>
#include <stdio.h>


#define EXT_LEN  3


/*
 * Segmented string compare for version or release strings.
 *
 * @param a		1st string
 * @param b		2nd string
 * @return		+1 if a is "newer", 0 if equal, -1 if b is "newer"
 */
int compare_versions (const char * a, const char * b);

char * var_to_pkg_config_var (const char *key, const char *var);
char * var_to_env_var (const char *key, const char *var);

gboolean is_str_one_text (const char *value);
gboolean is_str_true_text (const char *value);

gboolean name_ends_in_uninstalled (const char *str);

gboolean ends_in_dotpc (const char *str);

gboolean read_one_line (FILE *stream, GString *str);

void backslash_to_slash (char *p);

char * s_valid (const char *p);
char * s_module_sep (const char *p);
char * s_not_module_sep(const char *p);
char * scut_module_sep (char *p);
char * s_space (const char *p);
char * s_space_equal (const char *p);
char * s_not_space (const char *p);
char * scut_space (char *p);
char * scut_not_space_equal (char *p);
char * s_end_bracket (const char *p);

char * s_trim (const char *str);
char * s_dup_escape_shell (const char *str);


#endif  /* _STRUTIL_H_ */
