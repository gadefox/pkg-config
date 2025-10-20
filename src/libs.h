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

#ifndef _LIBS_H_
#define _LIBS_H_

#include "package.h"


void lib_init_system_dirs (Package *pkg_config);

void libs_verify (Package *pkg, Package *config);
gboolean lib_is_system_dirs (const char* compare, const char *key, const char *arg);
void lib_add_system_dirs (const gchar *dirs);

gboolean libs_parse (Package *pkg, Package *config, const char *str, const char *path);
gboolean libs_parse_private (Package *pkg, Package *config, const char *str, const char *path);


#endif  /* _LIBS_H_ */
