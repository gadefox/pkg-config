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

#ifndef _REQUIRE_VERSION_H_
#define _REQUIRE_VERSION_H_

#include "package.h"


typedef enum
{
  LESS_THAN,
  GREATER_THAN,
  LESS_THAN_EQUAL,
  GREATER_THAN_EQUAL,
  EQUAL,
  NOT_EQUAL,
  ALWAYS_MATCH,
  UNKNOWN               /* Used when we can't parse comparison */
} ComparisonType;

typedef struct
{
  char *name;
  ComparisonType comparison;
  char *version;
  Package *owner;
} RequiredVersion;


void required_version_free (RequiredVersion *rv);
RequiredVersion * required_version_create (Package *owner);

#if GLIB_CHECK_VERSION(2,28,0)
  #define required_version_free_list(l)   g_list_free_full ((l), (GDestroyNotify) required_version_free)
#else
  void required_version_free_list (GList *list);
#endif // GLIB_CHECK_VERSION

gboolean version_test (ComparisonType comparison,
                       const char *a,
                       const char *b);

const char *comparison_to_str (ComparisonType comparison);

void required_versions_add (Package *pkg, RequiredVersion *ver);

gboolean required_version_parse (RequiredVersion *ver, const char *value, const char* path);
ComparisonType parse_comparison_type (const char *value);


#endif  /* _REQUIRE_VERSION_H_ */
