/* GStreamer Plugins Cool
 * Copyright (C) 2014 LG Electronics, Inc.
 *	Author : Jeongseok Kim <jeongseok.kim@lge.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gstcool.h"

#include <stdlib.h>

static gboolean gst_cool_initialized = FALSE;

static void gst_cool_init_config (void);
static void gst_cool_load_configuration (void);
static void gst_cool_load_debug_configuration (void);

static GKeyFile *config = NULL;
GKeyFile *
gst_cool_get_configuration (void)
{
  return config;
}

gboolean
gst_cool_init_check (int *argc, char **argv[], GError ** err)
{
  gboolean res;

  // TODO: configuration should be loaded by given path
  // static const gchar *config_name[] = { "gstcool.conf", NULL };
  // static const gchar *env_config_name[] = { "GST_COOL_CONFIG_DIR", NULL };

  if (gst_cool_initialized) {
    GST_DEBUG ("already initialized gst-cool");
    return TRUE;
  }

  gst_cool_init_config ();

  /* set debug config before gst_init */
  gst_cool_load_debug_configuration ();

  res = gst_init_check (argc, argv, err);

  if (!res) {
    return FALSE;
  }

  gst_cool_load_configuration ();

  gst_cool_initialized = res;

  return res;
}

void
gst_cool_init (int *argc, char **argv[])
{
  GError *err = NULL;

  if (!gst_cool_init_check (argc, argv, &err)) {
    g_print ("Failed to initialize GStreamer Cool: %s\n",
        err ? err->message : "Unknown");
    if (err)
      g_error_free (err);
    exit (1);
  }
}

static void
gst_cool_init_config (void)
{
  GError *err = NULL;
  const gchar *cool_config_file;

  if ((cool_config_file = g_getenv ("GST_COOL_CONFIG")) != NULL) {
    GST_DEBUG ("loading gst-cool config from GST_COOL_CONFIG: %s",
        cool_config_file);
  } else {
    // FIXME: the path should come from configuration
    cool_config_file = "/etc/gst/gstcool.conf";
  }

  config = g_key_file_new ();

  if (!g_key_file_load_from_file (config, cool_config_file, G_KEY_FILE_NONE,
          &err)) {
    GST_ERROR ("Failed to load gst-cool configuration file(%s): %s",
        cool_config_file, err->message);
    g_error_free (err);
  }
}

static void
gst_cool_load_configuration (void)
{
  gchar **sections = NULL;
  gsize n_sections;

  gchar **rank_items = NULL;
  gsize n_rank_items;

  gint i;
  GError *err = NULL;

  sections = g_key_file_get_groups (config, &n_sections);

  // TODO: I don't know which sections are required for the future release.
  for (i = 0; i < n_sections; i++) {

    if (g_strcmp0 ("rank", sections[i]) != 0) {
      GST_DEBUG ("ignored %s section", sections[i]);
      continue;
    }
  }

  err = NULL;
  if (!(rank_items = g_key_file_get_keys (config, "rank", &n_rank_items, &err))) {
    GST_ERROR ("Unable to read plugin names for setting rank '%s'",
        err->message);
    g_error_free (err);
    goto done;
  }

  for (i = 0; i < n_rank_items; i++) {
    gint rank;
    err = NULL;
    rank = g_key_file_get_integer (config, "rank", rank_items[i], &err);

    if (err) {
      GST_ERROR ("Unable to read rank value for %s: %s", rank_items[i],
          err->message);
      g_error_free (err);
      continue;
    }

    gst_cool_set_rank (rank_items[i], rank);
  }

done:
  g_strfreev (rank_items);
  g_strfreev (sections);
}

static void
gst_cool_load_debug_configuration (void)
{
  const gchar *debug_mode;
  const gchar *gst_debug;
  const gchar *gst_debug_file;
  const gchar *gst_debug_dump_dot_dir;
  gint gst_debug_no_color = 0;

  GError *err = NULL;

  debug_mode = g_key_file_get_string (config, "debug", "DEBUG_MODE", &err);
  if (err) {
    GST_WARNING ("Unable to read debug mode: %s", err->message);
    g_error_free (err);
    err = NULL;
  }

  /* if debug mode is not enable, do not set debug environment variable */
  if (g_strcmp0 (debug_mode, "enable") != 0)
    return;

  gst_debug = g_key_file_get_string (config, "debug", "GST_DEBUG", &err);
  if (err) {
    GST_WARNING ("Unable to read GST_DEBUG option: %s", err->message);
    g_error_free (err);
    err = NULL;
  }

  gst_debug_file =
      g_key_file_get_string (config, "debug", "GST_DEBUG_FILE", &err);
  if (err) {
    GST_WARNING ("Unable to read GST_DEBUG_FILE option: %s", err->message);
    g_error_free (err);
    err = NULL;
  }

  gst_debug_dump_dot_dir = g_key_file_get_string (config, "debug",
      "GST_DEBUG_DUMP_DOT_DIR", &err);
  if (err) {
    GST_WARNING ("Unable to read GST_DEBUG_DUMP_DOT_DIR option: %s",
        err->message);
    g_error_free (err);
    err = NULL;
  }

  gst_debug_no_color = g_key_file_get_integer (config, "debug",
      "GST_DEBUG_NO_COLOR", &err);
  if (err) {
    GST_WARNING ("Unable to read GST_DEBUG_NO_COLOR option: %s", err->message);
    g_error_free (err);
    err = NULL;
  }

  GST_WARNING ("setenv GST_DEBUG=%s, GST_DEBUG_FILE=%s\n"
      "GST_DEBUG_DUMP_DOT_DIR=%s, GST_DEBUG_GST_DEBUG_NO_COLOR=%d",
      gst_debug, gst_debug_file, gst_debug_dump_dot_dir, gst_debug_no_color);

  if (gst_debug)
    g_setenv ("GST_DEBUG", gst_debug, 1);
  if (gst_debug_file)
    g_setenv ("GST_DEBUG_FILE", gst_debug_file, 1);
  if (gst_debug_dump_dot_dir)
    g_setenv ("GST_DEBUG_DUMP_DOT_DIR", gst_debug_dump_dot_dir, 1);
  if (gst_debug_no_color)
    g_setenv ("GST_DEBUG_NO_COLOR", "1", 1);
}

void
gst_cool_set_rank (const gchar * plugin, gint rank)
{
  GstPluginFeature *feature;
  feature =
      gst_registry_find_feature (gst_registry_get (), plugin,
      GST_TYPE_ELEMENT_FACTORY);

  if (!feature) {
    GST_INFO ("Unable to set rank of '%s' as %d", plugin, rank);
    return;
  }

  gst_plugin_feature_set_rank (feature, rank);

  GST_DEBUG ("'%s' has %d rank value", plugin, rank);

  gst_object_unref (feature);
}

void
gst_cool_set_decode_buffer_size (guint in_size, guint out_size)
{
  GKeyFile *config = gst_cool_get_configuration ();

  g_key_file_set_integer (config, "decode", "in_size", in_size);

  g_key_file_set_integer (config, "decode", "out_size", out_size);

  GST_DEBUG ("decode buffer has changed in: %d, out: %d", in_size, out_size);
}
