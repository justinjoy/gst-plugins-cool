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
#include "gstcoolrawcaps.h"

#include <stdlib.h>


static GstStaticCaps cool_raw_caps = GST_STATIC_CAPS (COOL_RAW_CAPS);
static gboolean gst_cool_initialized = FALSE;

static void gst_cool_load_configuration (const gchar * config_file);
static void gst_cool_set_rank (const gchar * plugin, gint rank);

static GKeyFile *config = NULL;
GKeyFile *
gst_cool_get_configuration (void)
{
  return config;
}

gboolean
gst_cool_init_check (int *argc, char **argv[], GError ** err)
{
  const gchar *cool_config_file;
  gboolean res;

  // TODO: configuration should be loaded by given path
  // static const gchar *config_name[] = { "gstcool.conf", NULL };
  // static const gchar *env_config_name[] = { "GST_COOL_CONFIG_DIR", NULL };

  res = gst_init_check (argc, argv, err);

  if (!res) {
    return FALSE;
  }

  if (gst_cool_initialized) {
    GST_DEBUG ("already initialized gst-cool");
    return TRUE;
  }

  if ((cool_config_file = g_getenv ("GST_COOL_CONFIG")) != NULL) {
    GST_DEBUG ("loading gst-cool config from GST_COOL_CONFIG: %s",
        cool_config_file);
  } else {
    // FIXME: the path should come from configuration
    cool_config_file = "/etc/gst/gstcool.conf";
  }

  gst_cool_load_configuration (cool_config_file);

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
gst_cool_load_configuration (const gchar * config_file)
{
  gchar **sections = NULL;
  gsize n_sections;

  gchar **rank_items = NULL;
  gsize n_rank_items;

  gint i;
  GError *err = NULL;

  config = g_key_file_new ();

  if (!g_key_file_load_from_file (config, config_file, G_KEY_FILE_NONE, &err)) {
    GST_ERROR ("Failed to load gst-cool configuration file(%s): %s",
        config_file, err->message);
    g_error_free (err);
    goto done;
  }

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

static void
playbin_element_added (GstElement * playbin, GstElement * element,
    gpointer user_data)
{
  if (!g_strrstr (GST_ELEMENT_NAME (element), "uridecodebin"))
    return;

  if (!g_object_class_find_property (G_OBJECT_GET_CLASS (element), "caps")) {
    GST_ERROR ("%s does not has caps property", GST_ELEMENT_NAME (element));
    return;
  }
  // FIXME: cool_raw_caps should come from configuration
  g_object_set (element, "caps", gst_static_caps_get (&cool_raw_caps), NULL);
}

gboolean
gst_cool_playbin_init (GstElement * playbin)
{
  g_return_val_if_fail (playbin != NULL, FALSE);

  // FIXME: reclaim connected signal handle when destorying playbin
  /* change default caps on uridecodebin */
  g_signal_connect (GST_BIN_CAST (playbin), "element-added",
      (GCallback) playbin_element_added, NULL);

  return TRUE;
}
