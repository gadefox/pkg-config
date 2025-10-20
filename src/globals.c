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

#include "globals.h"
#include "flag.h"
#include "package.h"


/* Hash table with package items and key pointers */
GHashTable *packages = NULL;

/* Hash table with allocated strings and keys */
GHashTable *globals = NULL;

/* List of allocated strings */
TailList search_dirs = { NULL, NULL };
TailList cflag_system_dirs = { NULL, NULL };
TailList lib_system_dirs = { NULL, NULL };

gboolean disable_uninstalled = FALSE;
gboolean ignore_requires = FALSE;
gboolean ignore_requires_private = TRUE;
gboolean ignore_private_libs = TRUE;

char *pcsysrootdir = NULL;
char *pkg_config_pc_path = NULL;

gboolean allow_system_cflags = FALSE;
gboolean allow_system_libs = FALSE;

gboolean want_my_version = FALSE;
gboolean want_version = FALSE;
FlagType pkg_flags = 0;
gboolean want_list = FALSE;
gboolean want_static_lib_list = ENABLE_INDIRECT_DEPS;
gboolean want_short_errors = FALSE;
gboolean want_uninstalled = FALSE;
char *variable_name = NULL;
gboolean want_exists = FALSE;
gboolean want_provides = FALSE;
gboolean want_requires = FALSE;
gboolean want_requires_private = FALSE;
gboolean want_validate = FALSE;
char *required_atleast_version = NULL;
char *required_exact_version = NULL;
char *required_max_version = NULL;
char *required_pkgconfig_version = NULL;
gboolean want_silence_errors = FALSE;
gboolean want_variable_list = FALSE;
gboolean want_debug_spew = FALSE;

#if HAVE_PARSE_SPEW
  gboolean want_parse_spew = FALSE;
#endif

gboolean want_verbose_errors = FALSE;
gboolean want_stdout_errors = FALSE;
gboolean output_opt_set = FALSE;

/*
 * Code
 */

void
enable_debug_spew (void)
{
  want_debug_spew = TRUE;
  want_verbose_errors = TRUE;
  want_silence_errors = FALSE;
}

#if HAVE_PARSE_SPEW

void
enable_parse_spew (void)
{
  want_parse_spew = TRUE;
}

#endif // HAVE_PARSE_SPEW

void
enable_private_libs(void)
{
  ignore_private_libs = FALSE;
}

void
disable_private_libs(void)
{
  ignore_private_libs = TRUE;
}

void
enable_requires(void)
{
  ignore_requires = FALSE;
}

void
disable_requires(void)
{
  ignore_requires = TRUE;
}

void
enable_requires_private(void)
{
  ignore_requires_private = FALSE;
}

void
disable_requires_private(void)
{
  ignore_requires_private = TRUE;
}
