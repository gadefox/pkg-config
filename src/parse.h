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

#ifndef _PARSE_H_
#define _PARSE_H_

#include "package.h"


#define OPERATOR_CHAR(c)    ((c) == '<' || (c) == '>' || (c) == '!' || (c) == '=')

#define IS_SPACE(c)         (isspace ((guchar)(c)))
#define MODULE_SEPARATOR(c) ((c) == ',' || IS_SPACE(c))
#define IS_VALID_CHAR(c)    (((c) >= 'A' && (c) <= 'Z') || ((c) >= 'a' && (c) <= 'z') \
                          || ((c) >= '0' && (c) <= '9') || (c) == '_' || (c) == '.')


extern gboolean parse_strict;


Package *parse_package_file (const char *key, const char *path,
                             Package *pkg_config,
                             gboolean ignore_requires,
                             gboolean ignore_private_libs,
                             gboolean ignore_requires_private,
                             gboolean *die);

GList   *parse_module_list (Package *pkg,
                            Package *config,
                            const char *str,
                            const char *path,
                            gboolean *die);

char * parse_package_variable (Package *pkg, Package *config, const char *variable);


#endif  /* _PARSE_H_ */
