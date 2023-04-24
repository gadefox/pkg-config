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

#ifndef _GLOBALS_H_
#define _GLOBALS_H_

#include <glib.h>
#include "flag.h" 
#include "taillist.h" 


/* Hash table with package items and key pointers */
extern GHashTable *packages;

/* Hash table with allocated strings and keys */
extern GHashTable *globals;

/* List of allocated strings */
extern TailList search_dirs;
extern TailList cflag_system_dirs;
extern TailList lib_system_dirs;

extern gboolean disable_uninstalled;
extern gboolean ignore_requires;
extern gboolean ignore_requires_private;
extern gboolean ignore_private_libs;

/* If TRUE, do not automatically prefer uninstalled versions */
extern gboolean disable_uninstalled;

extern gboolean allow_system_cflags;
extern gboolean allow_system_libs;

/* pkg-config default search path. On Windows the current pkg-config install
 * directory is used. Otherwise, the build-time defined PKG_CONFIG_PC_PATH.
 */
extern char *pcsysrootdir;
extern char *pkg_config_pc_path;

/* Exit on parse errors if TRUE. */
extern gboolean parse_strict;

/* If TRUE, define "prefix" in .pc files at runtime. */
extern gboolean define_prefix;

/* The name of the variable that acts as prefix, unless it is "prefix" */
extern char *prefix_variable;

#ifdef G_OS_WIN32
/* If TRUE, output flags in MSVC syntax. */
extern gboolean msvc_syntax;
#endif

extern gboolean want_my_version;
extern gboolean want_version;
extern FlagType pkg_flags;
extern gboolean want_list;
extern gboolean want_static_lib_list;
extern gboolean want_short_errors;
extern gboolean want_uninstalled;
extern char *variable_name;
extern gboolean want_exists;
extern gboolean want_provides;
extern gboolean want_requires;
extern gboolean want_requires_private;
extern gboolean want_validate;
extern char *required_atleast_version;
extern char *required_exact_version;
extern char *required_max_version;
extern char *required_pkgconfig_version;
extern gboolean want_silence_errors;
extern gboolean want_variable_list;
extern gboolean want_debug_spew;

#if HAVE_PARSE_SPEW
  extern gboolean want_parse_spew;
#endif

extern gboolean want_verbose_errors;
extern gboolean want_stdout_errors;
extern gboolean output_opt_set;


void enable_private_libs(void);
void disable_private_libs(void);
void enable_requires(void);
void disable_requires(void);
void enable_requires_private(void);
void disable_requires_private(void);

void enable_debug_spew (void);

#if HAVE_PARSE_SPEW
  void enable_parse_spew (void);
#endif


#endif  /* GLOBALS_H */
