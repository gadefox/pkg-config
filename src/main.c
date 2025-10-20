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

#include <locale.h>

#include "main.h"
#include "globals.h"
#include "package.h"
#include "parse.h"
#include "strutil.h"
#include "reqver.h"
#include "utils.h"


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
static const GOptionEntry options_table[] = {
  { "version", 0, G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK,
    &output_opt_cb, "output version of pkg-config", NULL },
  { "modversion", 0, G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK,
    &output_opt_cb, "output version for package", NULL },
  { "atleast-pkgconfig-version", 0, 0, G_OPTION_ARG_STRING,
    &required_pkgconfig_version,
    "require given version of pkg-config", "VERSION" },
  { "libs", 0, G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK, &output_opt_cb,
    "output all linker flags", NULL },
  { "static", 0, 0, G_OPTION_ARG_NONE, &want_static_lib_list,
    "output linker flags for static linking", NULL },
  { "short-errors", 0, 0, G_OPTION_ARG_NONE, &want_short_errors,
    "print short errors", NULL },
  { "libs-only-l", 0, G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK,
    &output_opt_cb, "output -l flags", NULL },
  { "libs-only-other", 0, G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK,
    &output_opt_cb, "output other libs (e.g. -pthread)", NULL },
  { "libs-only-L", 0, G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK,
    &output_opt_cb, "output -L flags", NULL },
  { "cflags", 0, G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK, &output_opt_cb,
    "output all pre-processor and compiler flags", NULL },
  { "cflags-only-I", 0, G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK,
    &output_opt_cb, "output -I flags", NULL },
  { "cflags-only-other", 0, G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK,
    &output_opt_cb, "output cflags not covered by the cflags-only-I option",
    NULL },
  { "variable", 0, 0, G_OPTION_ARG_CALLBACK, &output_opt_cb,
    "get the value of variable named NAME", "NAME" },
  { "define-variable", 0, 0, G_OPTION_ARG_CALLBACK, &define_variable_cb,
    "set variable NAME to VALUE", "NAME=VALUE" },
  { "exists", 0, G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK, &output_opt_cb,
    "return 0 if the module(s) exist", NULL },
  { "print-variables", 0, G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK,
    &output_opt_cb, "output list of variables defined by the module", NULL },
  { "uninstalled", 0, G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK,
    &output_opt_cb, "return 0 if the uninstalled version of one or more "
    "module(s) or their dependencies will be used", NULL },
  { "atleast-version", 0, 0, G_OPTION_ARG_CALLBACK, &output_opt_cb,
    "return 0 if the module is at least version VERSION", "VERSION" },
  { "exact-version", 0, 0, G_OPTION_ARG_CALLBACK, &output_opt_cb,
    "return 0 if the module is at exactly version VERSION", "VERSION" },
  { "max-version", 0, 0, G_OPTION_ARG_CALLBACK, &output_opt_cb,
    "return 0 if the module is at no newer than version VERSION", "VERSION" },
  { "list-all", 0, G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK,
    &output_opt_cb, "list all known packages", NULL },
  { "debug", 0, 0, G_OPTION_ARG_NONE, &want_debug_spew,
    "show verbose debug information", NULL },
  { "print-errors", 0, 0, G_OPTION_ARG_NONE, &want_verbose_errors,
    "show verbose information about missing or conflicting packages "
    "(default unless --exists or --atleast/exact/max-version given on the "
    "command line)", NULL },
  { "silence-errors", 0, 0, G_OPTION_ARG_NONE, &want_silence_errors,
    "be silent about errors (default when --exists or "
    "--atleast/exact/max-version given on the command line)", NULL },
  { "errors-to-stdout", 0, 0, G_OPTION_ARG_NONE, &want_stdout_errors,
    "print errors from --print-errors to stdout not stderr", NULL },
  { "print-provides", 0, G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK,
    &output_opt_cb, "print which packages the package provides", NULL },
  { "print-requires", 0, G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK,
    &output_opt_cb, "print which packages the package requires", NULL },
  { "print-requires-private", 0, G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK,
    &output_opt_cb, "print which packages the package requires for static "
    "linking", NULL },
  { "validate", 0, G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK,
    &output_opt_cb, "validate a package's .pc file", NULL },
  { "define-prefix", 0, 0, G_OPTION_ARG_NONE, &define_prefix,
    "try to override the value of prefix for each .pc file found with a "
    "guesstimated value based on the location of the .pc file", NULL },
  { "dont-define-prefix", 0, G_OPTION_FLAG_REVERSE, G_OPTION_ARG_NONE,
    &define_prefix, "don't try to override the value of prefix for each .pc "
    "file found with a guesstimated value based on the location of the .pc "
    "file", NULL },
  { "prefix-variable", 0, 0, G_OPTION_ARG_STRING, &prefix_variable,
    "set the name of the variable that pkg-config automatically sets",
    "PREFIX" },
#ifdef G_OS_WIN32
  { "msvc-syntax", 0, 0, G_OPTION_ARG_NONE, &msvc_syntax,
    "output -l and -L flags for the Microsoft compiler (cl)", NULL },
#endif
  { NULL, 0, 0, 0, NULL, NULL, NULL }
};
#pragma GCC diagnostic pop

/*
 * Code
 */

gboolean
define_variable_cb (const char *opt, const char *arg, gpointer data,
                    GError **error)
{
  char *varname;
  char *varval;
  char *newarg;
  gboolean success;

  newarg = g_strdup (arg);

  varname = s_space (newarg);

  varval = s_space_equal (varname);
  varval = scut_not_space_equal (varval);

  if (*varval == '\0')
    {
      spew ("--define-variable argument does not have a value for the variable\n", FALSE);

      success = FALSE;
      goto quit;
    }

  success = define_global_variable (varname, varval);

quit:

  g_free (newarg);
  return success;
}

gboolean
output_opt_cb (const char *opt, const char *arg, gpointer data,
               GError **error)
{
  static gboolean vercmp_opt_set = FALSE;
  gboolean bad_opt = TRUE;

  /* only allow one output mode, with a few exceptions */
  if (output_opt_set)
    {
      /* multiple flag options (--cflags --libs-only-l) allowed */
      if (pkg_flags != 0 &&
          (strcmp (opt, "--libs") == 0 ||
           strcmp (opt, "--libs-only-l") == 0 ||
           strcmp (opt, "--libs-only-other") == 0 ||
           strcmp (opt, "--libs-only-L") == 0 ||
           strcmp (opt, "--cflags") == 0 ||
           strcmp (opt, "--cflags-only-I") == 0 ||
           strcmp (opt, "--cflags-only-other") == 0))
        bad_opt = FALSE;

      /* --print-requires and --print-requires-private allowed */
      if ((want_requires && strcmp (opt, "--print-requires-private") == 0) ||
          (want_requires_private && strcmp (opt, "--print-requires") == 0))
        bad_opt = FALSE;

      /* --exists allowed with --atleast/exact/max-version */
      if (want_exists && !vercmp_opt_set &&
          (strcmp (opt, "--atleast-version") == 0 ||
           strcmp (opt, "--exact-version") == 0 ||
           strcmp (opt, "--max-version") == 0))
        bad_opt = FALSE;

      if (bad_opt)
        {
          spew ("Ignoring incompatible output option \"%s\"\n", opt);
          return TRUE;
        }
    }

  if (strcmp (opt, "--version") == 0)
    want_my_version = TRUE;
  else if (strcmp (opt, "--modversion") == 0)
    want_version = TRUE;
  else if (strcmp (opt, "--libs") == 0)
    pkg_flags |= LIBS_ANY;
  else if (strcmp (opt, "--libs-only-l") == 0)
    pkg_flags |= LIBS_l;
  else if (strcmp (opt, "--libs-only-other") == 0)
    pkg_flags |= LIBS_OTHER;
  else if (strcmp (opt, "--libs-only-L") == 0)
    pkg_flags |= LIBS_L;
  else if (strcmp (opt, "--cflags") == 0)
    pkg_flags |= CFLAGS_ANY;
  else if (strcmp (opt, "--cflags-only-I") == 0)
    pkg_flags |= CFLAGS_I;
  else if (strcmp (opt, "--cflags-only-other") == 0)
    pkg_flags |= CFLAGS_OTHER;
  else if (strcmp (opt, "--variable") == 0)
    variable_name = g_strdup (arg);
  else if (strcmp (opt, "--exists") == 0)
    want_exists = TRUE;
  else if (strcmp (opt, "--print-variables") == 0)
    want_variable_list = TRUE;
  else if (strcmp (opt, "--uninstalled") == 0)
    want_uninstalled = TRUE;
  else if (strcmp (opt, "--atleast-version") == 0)
    {
      required_atleast_version = g_strdup (arg);
      want_exists = TRUE;
      vercmp_opt_set = TRUE;
    }
  else if (strcmp (opt, "--exact-version") == 0)
    {
      required_exact_version = g_strdup (arg);
      want_exists = TRUE;
      vercmp_opt_set = TRUE;
    }
  else if (strcmp (opt, "--max-version") == 0)
    {
      required_max_version = g_strdup (arg);
      want_exists = TRUE;
      vercmp_opt_set = TRUE;
    }
  else if (strcmp (opt, "--list-all") == 0)
    want_list = TRUE;
  else if (strcmp (opt, "--print-provides") == 0)
    want_provides = TRUE;
  else if (strcmp (opt, "--print-requires") == 0)
    want_requires = TRUE;
  else if (strcmp (opt, "--print-requires-private") == 0)
    want_requires_private = TRUE;
  else if (strcmp (opt, "--validate") == 0)
    want_validate = TRUE;
  else
    return FALSE;

  output_opt_set = TRUE;
  return TRUE;
}

void
print_list_data (gpointer data,
                 gpointer user_data)
{
  g_print ("%s\n", (gchar *)data);
}

static Result
process_package_args (Package *config, const char *cmdline, GList **packages, FILE *log)
{
  GList *reqs;
  GList *curr;
  Package *req;
  RequiredVersion *ver;
  Result result = Success;
  gboolean temp;
  TailList new_packages;

  tail_list_init (new_packages);

  reqs = parse_module_list (NULL, config, cmdline, "(command line arguments)", &temp);
  if (reqs == NULL)
    {
      /*
       * In the previous version the application has been terminated when 'parse_strict'
       * is true. Now all functions in parse.c don't call exit (die) anymore but they set
       * the boolean e.g success in this case. This applicatin will terminate later with
       * 'success' result value set to false.
       */
      if ( temp )
        return Die;

      spew ("Must specify package names on the command line\n");

      return Error;
    }

  /* Everything is fine now, but we'll see later.. */
  for ( curr = reqs; curr != NULL; curr = curr->next )
    {
      ver = curr->data;

      /* override requested versions with cmdline options */
      if (required_exact_version)
        {
          g_free (ver->version);
          ver->comparison = EQUAL;
          ver->version = g_strdup (required_exact_version);
        }
      else if (required_atleast_version)
        {
          g_free (ver->version);
          ver->comparison = GREATER_THAN_EQUAL;
          ver->version = g_strdup (required_atleast_version);
        }
      else if (required_max_version)
        {
          g_free (ver->version);
          ver->comparison = LESS_THAN_EQUAL;
          ver->version = g_strdup (required_max_version);
        }

      req = package_get (config, ver->name, !want_short_errors, disable_uninstalled, &temp );
      if (req == NULL)
        {
          if ( temp )
            goto quit;

          if ( log != NULL )
            fprintf (log, "%s NOT-FOUND\n", ver->name);

          result = Error;
          verbose_error ("No package '%s' found\n", ver->name);
          continue;
        }

      if (log != NULL)
        {
          fprintf (log, "%s %s %s\n", ver->name,
                   comparison_to_str (ver->comparison),
                   (ver->version == NULL) ? "(null)" : ver->version);
        }

      if (!version_test (ver->comparison, req->version, ver->version))
        {
          result = Error;

          verbose_error ("Requested '%s %s %s' but version of %s is %s\n",
                         ver->name,
                         comparison_to_str (ver->comparison),
                         ver->version,
                         req->name,
                         req->version);

          if (req->url)
            verbose_error ("You may find new versions of %s at %s\n",
                           req->name, req->url);

          /* We don't need to destroy structure because it's added to hash table */
          continue;
        }

      tail_list_add (&new_packages, req);
    }

  /* We don't need this list anymore so release the memory */
  required_version_free_list (reqs);

  *packages = new_packages.items;

  return result;

quit:

  /* We don't need this list anymore so release the memory */
  g_list_free (new_packages.items);
  required_version_free_list (reqs);

  return Die;
}

/* process Requires.private: */
static void
handle_package_requires_private ( Package *pkg )
{
  GList *iter;
  Package *deppkg;
  RequiredVersion *req;

  for (iter = pkg->requires_private.items; iter != NULL; iter = iter->next)
    {
      deppkg = iter->data;

      if (g_list_find (pkg->requires.items, deppkg))
        continue;

      req = g_hash_table_lookup(pkg->required_versions, deppkg->key);
      if ( req == NULL || req->comparison == ALWAYS_MATCH )
        printf ("%s\n", deppkg->key);
      else
        printf ("%s %s %s\n", deppkg->key,
          comparison_to_str(req->comparison),
          req->version);
    }
}

static void
handle_requires_private ( GList *packages )
{
  Package *pkg;
  GList *iter;

  if (!want_requires_private)
    return;

  for (iter = packages; iter != NULL; iter = iter->next)
    {
      pkg = iter->data;

      handle_package_requires_private (pkg);
    }
}

static void
handle_package_requires ( Package *pkg )
{
  GList *iter;
  Package *deppkg;
  RequiredVersion *req;

  /* process Requires: */
  for (iter = pkg->requires.items; iter != NULL; iter = iter->next)
    {
      deppkg = iter->data;

      req = g_hash_table_lookup(pkg->required_versions, deppkg->key);
      if ( req == NULL || req->comparison == ALWAYS_MATCH )
        printf ("%s\n", deppkg->key);
      else
        printf ("%s %s %s\n", deppkg->key,
          comparison_to_str(req->comparison),
          req->version);
    }
}

static void
handle_requires ( GList *packages )
{
  GList *iter;
  Package *pkg;

  if (!want_requires)
    return;

  for (iter = packages; iter != NULL; iter = iter->next)
    {
      pkg = iter	->data;

      handle_package_requires (pkg);
    }
}

static void
handle_provides ( GList *packages )
{
  GList *iter;
  Package *pkg;
  char *key;

  if (!want_provides)
    return;

  for ( iter = packages; iter != NULL; iter = iter->next )
    {
      pkg = iter->data;

      key = pkg->key;
      while (*key == '/')
        key++;

      if ( *key != '\0' )
        printf ("%s = %s\n", key, pkg->version);
   }
}

static void
handle_version ( GList *packages )
{
  GList *iter;
  Package *pkg;

  if (!want_version)
    return;

  for ( iter = packages; iter != NULL; iter = iter->next )
    {
      pkg = iter->data;

      printf ("%s\n", pkg->version);
    }
}

static Result
handle_uninstalled ( GList *packages )
{
  GList *iter;
  Package *pkg;

  if (!want_uninstalled)
    return IgnoreUninstalled;

  /* See if > 0 pkgs (including dependencies recursively) were uninstalled */
  for ( iter = packages; iter != NULL; iter = iter->next )
    {
      pkg = iter->data;

      if (package_uninstalled (pkg))
        return Success;
    }

  return Error;
}

static gboolean
handle_variable_list ( GList *packages )
{
  GList *iter;
  Package *pkg;
  GList *keys;
  gboolean first = TRUE;

  if (!want_variable_list)
    return FALSE;

  for ( iter = packages; iter != NULL; iter = iter->next )
    {
      pkg = iter->data;
      if (pkg->vars == NULL)
        continue;

      /* Print newline after each package block except the last,
       * so we print newline before the the block except the first
       * block */
      if ( first )
        first = FALSE;
      else
        printf ("\n");

      /* Sort variables for consistent output */
      keys = g_hash_table_get_keys (pkg->vars);
      keys = g_list_sort (keys, (GCompareFunc)g_strcmp0);
      g_list_foreach (keys, print_list_data, NULL);
      g_list_free (keys);
    }

  return TRUE;
}

static gboolean
handle_env_vars ( Package *pkg_config )
{
  const char *var;

  /* PKG_CONFIG_PATH */
  var = getenv ("PKG_CONFIG_PATH");
  if (var != NULL)
    add_search_dirs(var, G_SEARCHPATH_SEPARATOR_S, "PKG_CONFIG_PATH");

  /* PKG_CONFIG_LIBDIR */
  var = getenv ("PKG_CONFIG_LIBDIR");
  if (var != NULL)
    add_search_dirs (var, G_SEARCHPATH_SEPARATOR_S, "PKG_CONFIG_LIBDIR");
  else
    add_search_dirs (pkg_config_pc_path, G_SEARCHPATH_SEPARATOR_S, "pkg-config package");

  /* PKG_CONFIG_SYSROOT_DIR */
  pcsysrootdir = getenv ("PKG_CONFIG_SYSROOT_DIR");
  if (pcsysrootdir == NULL)
    pcsysrootdir = package_get_var( pkg_config, "sysrootdir" );

  var = pcsysrootdir != NULL ? pcsysrootdir : "/";
  if (!define_global_variable ("pc_sysrootdir", var))
    return FALSE;

  /* PKG_CONFIG_TOP_BUILD_DIR */
  var = getenv ("PKG_CONFIG_TOP_BUILD_DIR");
  if (var == NULL)
    {
      var = package_get_var( pkg_config, "topbuilddir" );
      if (var == NULL)
        var = "$(top_builddir)";
    }

  if (!define_global_variable ("pc_top_builddir", var))
    return FALSE;

  /* PKG_CONFIG_DISABLE_UNINSTALLED */
  if (getenv ("PKG_CONFIG_DISABLE_UNINSTALLED") != NULL || package_get_varval_bool( pkg_config, "disable_uninstalled" ) )
    {
      debug_spew ("disabling auto-preference for uninstalled packages\n");
      disable_uninstalled = TRUE;
    }

  /* Allow system flags */
  if ( getenv ("PKG_CONFIG_ALLOW_SYSTEM_CFLAGS") != NULL )
      allow_system_cflags = TRUE;
  else
      allow_system_cflags = package_get_varval_bool (pkg_config, "allow_system_cflags");

  /* Allow system libs */
  if ( getenv ("PKG_CONFIG_ALLOW_SYSTEM_LIBS") != NULL )
      allow_system_libs = TRUE;
  else
      allow_system_libs = package_get_varval_bool (pkg_config, "allow_system_libs");

  return TRUE;
}

static Result
handle_args ( Package *pkg_config, int argc, char **argv, GList **packages )
{
  char *path;
  FILE *log = NULL;
  GString *str;
  Result result;

  path = getenv("PKG_CONFIG_LOG");
  if ( path == NULL )
    path = package_get_var( pkg_config, "log" );

  if (path != NULL)
    {
      log = fopen (path, "a");
      if (log == NULL)
        {
          spew ("Cannot open log file: %s\n", path);

          /*
           * Ad 'die (1);' - instead of terminating applicatin here, let's
           * return null pointer and we will not continue;
           */
          return -1;
        }
    }

  /* Collect packages from remaining args */
  str = g_string_new ("");
  while (argc > 1)
    {
      argc--;
      argv++;

      g_string_append (str, *argv);
      g_string_append (str, " ");
    }

  g_strstrip (str->str);

  /* find and parse each of the packages specified */
  result = process_package_args (pkg_config, str->str, packages, log );

  g_string_free (str, TRUE);

  if ( log != NULL )
    fclose (log);

  return result;
}

static GOptionContext *
handle_options ( int *argc, char ***argv )
{
  GOptionContext *opt_context;
  GError *error = NULL;

  opt_context = g_option_context_new (NULL);
  g_option_context_add_main_entries (opt_context, options_table, NULL);
  if (!g_option_context_parse(opt_context, argc, argv, &error))
    {
      spew ( "%s\n", error->message );

      g_clear_error (&error);
      g_option_context_free (opt_context);

      return NULL;
    }

  /* If no output option was set, then --exists is the default. */
  if (!output_opt_set)
    {
      debug_spew ("no output option set, defaulting to --exists\n");
      want_exists = TRUE;
    }

  /* Error printing is determined as follows:
   *     - for --exists, --*-version, --list-all and no options at all,
   *       it's off by default and --print-errors will turn it on
   *     - for all other output options, it's on by default and
   *       --silence-errors can turn it off
   */
  if (want_exists || want_list)
    {
      debug_spew ("Error printing disabled by default due to use of output "
                  "options --exists, --atleast/exact/max-version, "
                  "--list-all or no output option at all. Value of "
                  "--print-errors: %d\n",
                  want_verbose_errors);

      /* Leave want_verbose_errors unchanged, reflecting --print-errors */
    }
  else
    {
      debug_spew ("Error printing enabled by default due to use of output "
                  "options besides --exists, --atleast/exact/max-version or "
                  "--list-all. Value of --silence-errors: %d\n",
                  want_silence_errors);

      if (want_silence_errors && !want_debug_spew)
        want_verbose_errors = FALSE;
      else
        want_verbose_errors = TRUE;
    }

  if (want_verbose_errors)
    debug_spew ("Error printing enabled\n");
  else
    debug_spew ("Error printing disabled\n");

  if (want_static_lib_list)
    enable_private_libs();
  else
    disable_private_libs();

  /* honor Requires.private if any Cflags are requested or any static
   * libs are requested */
  if (pkg_flags & CFLAGS_ANY || want_requires_private || want_exists ||
      (want_static_lib_list && (pkg_flags & LIBS_ANY)))
    {
      enable_requires_private();
    }

  /* ignore Requires if no Cflags or Libs are requested */

  if (pkg_flags == 0 && !want_requires && !want_exists)
    disable_requires();

  /* Allow errors in .pc files when listing all. */
  if (want_list)
    parse_strict = FALSE;

  return opt_context;
}

static int
handle ( Package *pkg_config, int argc, char **argv )
{
  GOptionContext *opt_context;
  Result result;
  GList *packages = NULL;
  gboolean need_newline = FALSE;
  char *str;

  /* Parse options */
  opt_context = handle_options( &argc, &argv );
  if ( opt_context == NULL )
    return 1;

  if (want_my_version)
    {
      printf ("%s\n", VERSION);
      goto quit;
    }

  if (required_pkgconfig_version)
    {
      if (compare_versions (VERSION, required_pkgconfig_version) >= 0)
        goto quit;

      goto error;
    }

  if (want_list)
    {
      if ( !scan_dirs ( pkg_config ) )
        goto error;

      package_print_list ( pkg_config );
      goto quit;
    }

  result = handle_args ( pkg_config, argc, argv, &packages );
  if ( result != Success )
    goto error;

  /* If the user just wants to check package existence or validate its .pc
   * file, we're all done. */
  if (want_exists || want_validate)
    goto quit;

  if ( handle_variable_list ( packages ) )
    need_newline = FALSE;

  result = handle_uninstalled ( packages );
  if ( result == Success ) goto quit;
  if ( result == Error )   goto error;

  /* Value is IgnoreUninstalled so we can continue */
  handle_version ( packages );
  handle_provides ( packages );
  handle_requires ( packages );
  handle_requires_private ( packages );

  /* Print all flags; then print a newline at the end. */
  if (variable_name)
    {
      str = packages_get_var (pkg_config, packages, variable_name);
      printf ("%s", str);
      g_free (str);
      need_newline = TRUE;
    }

  if (pkg_flags != 0)
    {
      str = flags_packages_get (packages, pkg_flags);
      printf ("%s", str);
      g_free (str);
      need_newline = TRUE;
    }

  if (need_newline)
    printf ("\n");

quit:

  g_list_free (packages);
  g_option_context_free (opt_context);

  return 0;

error:

  g_list_free (packages);
  g_option_context_free (opt_context);

  return 1;
}

int
main (int argc, char **argv)
{
  Package *pkg_config;
  int result = 0;

  setlocale (LC_CTYPE, "");
#ifdef LC_MESSAGES
  setlocale (LC_MESSAGES, "");
#endif

  /* This is here so that we get debug spew from the start,
   * during arg parsing. Please note the following code has to be included
   * at the beginning of application */
  if ( getenv ("PKG_CONFIG_DEBUG_SPEW") != NULL )
    {
      enable_debug_spew ();
      debug_spew ("PKG_CONFIG_DEBUG_SPEW variable enabling debug spew\n");
    }

  /* Initialize package variables and try to load package file */
  pkg_config = packages_initialize ( );
  if ( pkg_config != NULL )
    {
      /* Handle environment variables */
      if ( handle_env_vars( pkg_config ) )
        result = handle( pkg_config, argc, argv );
    }

  release( );

  return result;
}

