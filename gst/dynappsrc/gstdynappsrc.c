/* GStreamer Dynamic App Source element
 * Copyright (C) 2014 LG Electronics, Inc.
 *  Author : Wonchul Lee <wonchul86.lee@lge.com>
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


/**
 * SECTION:element-dynappsrc
 *
 * Dynappsrc provides multiple appsrc elements inside a single source element(bin)
 * for separated audio, video and text stream.
 *
 * <refsect2>
 * <title>Usage</title>
 * <para>
 * A dynappsrc element can be created by pipeline using URI included dynappsrc://
 * protocal.
 * The URI to play should be set via the #GstLpBin:uri property.
 *
 * Dynappsrc is #GstBin. It will notified to application when it is created by
 * source-setup signal of pipeline.
 * A appsrc element can be created by new-appsrc signal action to dynappsrc. It
 * should be created before changing state READY to PAUSED. Therefore application
 * need to create appsrc element as soon as receiving source-setup signal.
 * Then appsrc can be configured by setting the element to PAUSED state.
 *
 * When playback has finished (an EOS message has been received on the bus)
 * or an error has occured (an ERROR message has been received on the bus) or
 * the user wants to play a different track, dynappsrc should be set back to
 * READY or NULL state, then appsrc elements in dynappsrc should be set to the
 * NULL state and removed from it.
 * </para>
 * </refsect2>
 * <refsect2>
 * <title>Rule for reference appsrc</title>
 * <para>
 * Here is a rule for application when using the appsrc element.
 * When application created appsrc element, ref-count of appsrc is one by
 * default. If it was not intended to decrease ref-count during using by
 * application, pipeline may broke.
 * Therefore it should increase ref-count explicitly by using gst_object_ref.
 * It should be decreased ref-count either by using gst_object_unref when playback
 * has finished.
 * </para>
 * </refsect2>
 * <refsect2>
 * <title>Examples</title>
 * |[
 * app->lpbin = gst_element_factory_make ("lpbin", NULL);
 * g_object_set (app->lpbin, "uri", "dynappsrc://", NULL);
 * g_signal_connect (app->lpbin, "deep-notify::source",
 *     (GCallback) found_source, app);
 * ]|
 * This will create dynappsrc element in pipeline and watch source notify.
 * |[
 * gst_object_ref (appsrc1);
 * gst_object_ref (appsrc2);
 *
 * g_signal_emit_by_name (dynappsrc, "new-appsrc", &appsrc1);
 * g_signal_emit_by_name (dynappsrc, "new-appsrc", &appsrc2);
 *
 * g_signal_connect (app->appsrc1, "need-data", G_CALLBACK (start_feed), app);
 * g_signal_connect (app->appsrc1, "enough-data", G_CALLBACK (stop_feed), app);
 *
 * g_signal_connect (app->appsrc2, "need-data", G_CALLBACK (start_feed), app);
 * g_signal_connect (app->appsrc2, "enough-data", G_CALLBACK (stop_feed), app);
 * ]|
 * This will create appsrc elements when called source notify handler.
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "gstdynappsrc.h"

GST_DEBUG_CATEGORY_STATIC (dyn_appsrc_debug);
#define GST_CAT_DEFAULT dyn_appsrc_debug

#define parent_class gst_dyn_appsrc_parent_class

#define DEFAULT_PROP_URI NULL

#define PTS_DTS_MAX_VALUE (((guint64)1) << 33)
#define MPEGTIME_TO_GSTTIME(t) ((t) * 100000 / 9)

enum
{
  PROP_0,
  PROP_URI,
  PROP_N_SRC,
  PROP_SMART_PROPERTIES,
  PROP_LAST
};

enum
{
  SIGNAL_NEW_APPSRC,

  /* actions */
  SIGNAL_END_OF_STREAM,
  LAST_SIGNAL
};

static GstStaticPadTemplate src_template = GST_STATIC_PAD_TEMPLATE ("src_%u",
    GST_PAD_SRC,
    GST_PAD_SOMETIMES,
    GST_STATIC_CAPS_ANY);

static guint gst_dyn_appsrc_signals[LAST_SIGNAL] = { 0 };

static void gst_dyn_appsrc_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_dyn_appsrc_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static void gst_dyn_appsrc_finalize (GObject * self);
static GstStateChangeReturn gst_dyn_appsrc_change_state (GstElement * element,
    GstStateChange transition);
static GstElement *gst_dyn_appsrc_new_appsrc (GstDynAppSrc * bin,
    const gchar * name);
static void gst_dyn_appsrc_uri_handler_init (gpointer g_iface,
    gpointer iface_data);

G_DEFINE_TYPE_WITH_CODE (GstDynAppSrc, gst_dyn_appsrc, GST_TYPE_BIN,
    G_IMPLEMENT_INTERFACE (GST_TYPE_URI_HANDLER,
        gst_dyn_appsrc_uri_handler_init));


static void
gst_dyn_appsrc_class_init (GstDynAppSrcClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gstelement_class = GST_ELEMENT_CLASS (klass);

  gobject_class->set_property = gst_dyn_appsrc_set_property;
  gobject_class->get_property = gst_dyn_appsrc_get_property;
  gobject_class->finalize = gst_dyn_appsrc_finalize;

  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&src_template));

  g_object_class_install_property (gobject_class, PROP_URI,
      g_param_spec_string ("uri", "URI",
          "URI to get protected content",
          NULL, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * GstDynAppSrc:n-src
   *
   * Get the total number of available streams.
   */
  g_object_class_install_property (gobject_class, PROP_N_SRC,
      g_param_spec_int ("n-source", "Number Source",
          "Total number of source streams", 0, G_MAXINT, 0,
          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_SMART_PROPERTIES,
      g_param_spec_boxed ("smart-properties", "Smart Properties",
          "Hold various property values for reply custom query",
          GST_TYPE_STRUCTURE, G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS));

  /**
   * GstDynAppSrc::new-appsrc
   * @dynappsrc: a #GstDynAppSrc
   * @name : name of appsrc element
   *
   * Action signal to create a appsrc element.
   * This signal should be emitted before changing state READY to PAUSED.
   * The application emit this signal as soon as receiving source-setup signal from pipeline.
   *
   * Returns: a GstElement of appsrc element or NULL when element creation failed.
   */
  gst_dyn_appsrc_signals[SIGNAL_NEW_APPSRC] =
      g_signal_new ("new-appsrc", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
      G_STRUCT_OFFSET (GstDynAppSrcClass, new_appsrc), NULL, NULL,
      g_cclosure_marshal_generic, GST_TYPE_ELEMENT, 1, G_TYPE_STRING);

  /**
    * GstDynAppSrc::end-of-stream:
    * @dynappsrc: the dynappsrc
    *
    * Notify all of @appsrc that no more buffer are available.
    */
  gst_dyn_appsrc_signals[SIGNAL_END_OF_STREAM] =
      g_signal_new ("end-of-stream", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION, G_STRUCT_OFFSET (GstDynAppSrcClass,
          end_of_stream), NULL, NULL, g_cclosure_marshal_generic,
      GST_TYPE_FLOW_RETURN, 0, G_TYPE_NONE);

  klass->new_appsrc = gst_dyn_appsrc_new_appsrc;
  klass->end_of_stream = gst_dyn_appsrc_end_of_stream;

  gstelement_class->change_state =
      GST_DEBUG_FUNCPTR (gst_dyn_appsrc_change_state);

  gst_element_class_set_static_metadata (gstelement_class,
      "Dynappsrc", "Source/Bin",
      "Dynamic App Source", "Wonchul Lee <wonchul86.lee@lge.com>");

  GST_DEBUG_CATEGORY_INIT (dyn_appsrc_debug, "dynappsrc", 0,
      "Dynamic App Source");
}

static void
gst_dyn_appsrc_init (GstDynAppSrc * bin)
{
  /* init member variable */
  bin->uri = g_strdup (DEFAULT_PROP_URI);
  bin->appsrc_list = NULL;
  bin->n_source = 0;
  bin->smart_prop = NULL;

  bin->directv_rvu = FALSE;
  bin->segment_event = NULL;
  bin->rate = 1.0;
  gst_segment_init (&bin->segment, GST_FORMAT_TIME);

  GST_OBJECT_FLAG_SET (bin, GST_ELEMENT_FLAG_SOURCE);
}

static gboolean
set_smart_properties (GQuark field_id, const GValue * value, gpointer user_data)
{
  GstStructure *smart_prop = (GstStructure *) user_data;

  gst_structure_id_set_value (smart_prop, field_id, value);
  return TRUE;
}

static void
apply_smart_properties (GstAppSourceGroup * appsrc_group, GstDynAppSrc * bin)
{
  GstElement *elem = appsrc_group->appsrc;
  g_object_set (elem, "smart-properties", bin->smart_prop, NULL);
}

static void
gst_dyn_appsrc_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstDynAppSrc *bin = GST_DYN_APPSRC (object);

  switch (prop_id) {
    case PROP_URI:
    {
      GstURIHandler *handler;
      GstURIHandlerInterface *iface;

      handler = GST_URI_HANDLER (bin);
      iface = GST_URI_HANDLER_GET_INTERFACE (handler);
      iface->set_uri (handler, g_value_get_string (value), NULL);
    }
      break;
    case PROP_SMART_PROPERTIES:
    {
      const GstStructure *s = gst_value_get_structure (value);

      if (bin->smart_prop)
        gst_structure_foreach (s, set_smart_properties, bin->smart_prop);
      else
        bin->smart_prop = gst_structure_copy (s);

      if (bin->appsrc_list)
        g_list_foreach (bin->appsrc_list, (GFunc) apply_smart_properties, bin);

      gst_structure_get_boolean (bin->smart_prop, "directv-rvu",
          &bin->directv_rvu);
      break;
    }
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_dyn_appsrc_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstDynAppSrc *bin = GST_DYN_APPSRC (object);

  switch (prop_id) {
    case PROP_URI:
      g_value_set_string (value, bin->uri);
      break;
    case PROP_N_SRC:
    {
      GST_OBJECT_LOCK (bin);
      g_value_set_int (value, bin->n_source);
      GST_OBJECT_UNLOCK (bin);
      break;
    }
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_dyn_appsrc_finalize (GObject * self)
{
  GstDynAppSrc *bin = GST_DYN_APPSRC (self);

  g_free (bin->uri);

  if (bin->smart_prop) {
    gst_structure_free (bin->smart_prop);
    bin->smart_prop = NULL;
  }

  if (bin->segment_event) {
    gst_event_unref (bin->segment_event);
    bin->segment_event = NULL;
  }

  G_OBJECT_CLASS (parent_class)->finalize (self);
}

static gboolean
gst_dyn_appsrc_handle_src_query (GstPad * pad, GstObject * parent,
    GstQuery * query)
{
  GstPad *target = gst_ghost_pad_get_target (GST_GHOST_PAD_CAST (pad));
  gboolean res = FALSE;

  /* forward the query to the proxy target pad */
  if (target) {
    res = gst_pad_query (target, query);
    gst_object_unref (target);
  }

  return res;
}

static gboolean
gst_dyn_appsrc_handle_src_event (GstPad * pad, GstObject * parent,
    GstEvent * event)
{
  gboolean res = TRUE;
  GstPad *target;
  GstDynAppSrc *bin = GST_DYN_APPSRC (parent);
  GstIterator *it;
  GValue data = { 0, };

  /*
   * dynappsrc handle a seek event that it send to all of linked appsrce elements.
   */
  if (GST_EVENT_TYPE (event) == GST_EVENT_SEEK) {
    if (bin->directv_rvu) {
      GstFormat format;
      GstSeekFlags flags;
      gboolean flush;
      GstSeekType start_type, stop_type;
      gint64 start, stop;

      gst_event_parse_seek (event, &bin->rate, &format, &flags, &start_type,
          &start, &stop_type, &stop);

      flush = flags & GST_SEEK_FLAG_FLUSH;

      if (bin->segment_event) {
        gst_event_unref (bin->segment_event);
        bin->segment_event = NULL;
      }

      if (bin->rate >= 2 || bin->rate < 0 || (bin->rate == 1 && !flush)) {
        gst_event_unref (event);
        return TRUE;
      }

      if (start_type == GST_SEEK_TYPE_SET) {
        gst_event_unref (event);
        event = gst_event_new_seek (bin->rate, format, flags,
            GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE,
            GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE);
      }
    }

    it = gst_element_iterate_src_pads (GST_ELEMENT_CAST (bin));
    while (gst_iterator_next (it, &data) == GST_ITERATOR_OK) {
      GstPad *srcpad = g_value_get_object (&data);
      target = gst_ghost_pad_get_target (GST_GHOST_PAD_CAST (srcpad));
      res = gst_pad_send_event (target, gst_event_ref (event));
      gst_object_unref (target);
      g_value_reset (&data);
    }
    gst_event_unref (event);
    g_value_unset (&data);
    gst_iterator_free (it);
  } else if ((target = gst_ghost_pad_get_target (GST_GHOST_PAD_CAST (pad)))) {
    res = gst_pad_send_event (target, event);
    gst_object_unref (target);
  } else
    gst_event_unref (event);

  return res;
}

static GstPadProbeReturn
pad_probe_cb (GstPad * pad, GstPadProbeInfo * info, gpointer data)
{
  GstDynAppSrc *bin = GST_DYN_APPSRC (GST_PAD_PARENT (pad));
  GstPadProbeReturn ret = GST_PAD_PROBE_OK;
  GstEvent *event = GST_PAD_PROBE_INFO_EVENT (info);

  if (bin->directv_rvu && bin->n_source >= 2) {
    switch (GST_EVENT_TYPE (event)) {
      case GST_EVENT_FLUSH_STOP:
        if (bin->segment_event) {
          gst_event_unref (bin->segment_event);
          bin->segment_event = NULL;
        }
        gst_segment_init (&bin->segment, GST_FORMAT_TIME);
        break;
      case GST_EVENT_SEGMENT:
        GST_OBJECT_LOCK (bin);

        if (!bin->segment_event) {
          GList *item;
          for (item = bin->appsrc_list; item; item = g_list_next (item)) {
            GstAppSourceGroup *appsrc_group = (GstAppSourceGroup *) item->data;

            if (pad == appsrc_group->srcpad) {
              gst_event_copy_segment (event, &bin->segment);
              bin->segment.start = (bin->rate < 0) ? 0 : bin->segment.time;
              if (bin->rate < 0 && bin->segment.time < 2000000000)
                bin->segment.time += MPEGTIME_TO_GSTTIME (PTS_DTS_MAX_VALUE);
              bin->segment.stop =
                  (bin->rate < 0) ? bin->segment.time : GST_CLOCK_TIME_NONE;
              bin->segment.position = bin->segment.time;
              bin->segment.format = GST_FORMAT_TIME;
              bin->segment.rate = bin->rate;
              bin->segment_event = gst_event_new_segment (&bin->segment);
            }
          }
        }

        gst_event_make_writable (event);
        gst_event_ref (bin->segment_event);
        GST_PAD_PROBE_INFO_DATA (info) = bin->segment_event;
        gst_event_unref (event);

        GST_OBJECT_UNLOCK (bin);
        break;
      default:
        break;
    }
  }

  return ret;
}

static gboolean
setup_source (GstDynAppSrc * bin)
{
  GList *item;
  GstPadTemplate *pad_tmpl;
  gchar *padname;
  gboolean ret = FALSE;

  pad_tmpl = gst_static_pad_template_get (&src_template);

  for (item = bin->appsrc_list; item; item = g_list_next (item)) {
    GstAppSourceGroup *appsrc_group = (GstAppSourceGroup *) item->data;
    GstPad *srcpad = NULL;

    gst_bin_add (GST_BIN_CAST (bin), appsrc_group->appsrc);

    srcpad = gst_element_get_static_pad (appsrc_group->appsrc, "src");
    padname =
        g_strdup_printf ("src_%u", g_list_position (bin->appsrc_list, item));
    appsrc_group->srcpad =
        gst_ghost_pad_new_from_template (padname, srcpad, pad_tmpl);
    gst_pad_set_event_function (appsrc_group->srcpad,
        gst_dyn_appsrc_handle_src_event);
    gst_pad_set_query_function (appsrc_group->srcpad,
        gst_dyn_appsrc_handle_src_query);

    gst_pad_set_active (appsrc_group->srcpad, TRUE);
    gst_element_add_pad (GST_ELEMENT_CAST (bin), appsrc_group->srcpad);

    gst_pad_add_probe (appsrc_group->srcpad,
        GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM | GST_PAD_PROBE_TYPE_EVENT_FLUSH,
        pad_probe_cb, NULL, NULL);

    gst_object_unref (srcpad);
    g_free (padname);

    ret = TRUE;
  }
  gst_object_unref (pad_tmpl);

  if (ret) {
    GST_DEBUG_OBJECT (bin, "all appsrc elements are added");
    gst_element_no_more_pads (GST_ELEMENT_CAST (bin));
  }

  return ret;
}

static void
remove_source (GstDynAppSrc * bin)
{
  GList *item;

  for (item = bin->appsrc_list; item; item = g_list_next (item)) {
    GstAppSourceGroup *appsrc_group = (GstAppSourceGroup *) item->data;

    GST_DEBUG_OBJECT (bin, "removing appsrc element and ghostpad");

    gst_ghost_pad_set_target (GST_GHOST_PAD_CAST (appsrc_group->srcpad), NULL);
    gst_element_remove_pad (GST_ELEMENT_CAST (bin), appsrc_group->srcpad);
    appsrc_group->srcpad = NULL;

    gst_element_set_state (appsrc_group->appsrc, GST_STATE_NULL);
    gst_bin_remove (GST_BIN_CAST (bin), appsrc_group->appsrc);
    gst_object_unref (appsrc_group->appsrc);
    appsrc_group->appsrc = NULL;
  }

  g_list_free_full (bin->appsrc_list, g_free);
  bin->appsrc_list = NULL;

  bin->n_source = 0;
}

static GstElement *
gst_dyn_appsrc_new_appsrc (GstDynAppSrc * bin, const gchar * name)
{
  GstAppSourceGroup *appsrc_group;

  GST_OBJECT_LOCK (bin);

  if (GST_STATE (bin) >= GST_STATE_PAUSED) {
    GST_WARNING_OBJECT (bin,
        "deny to create appsrc when state is in PAUSED or PLAYING state");
    GST_OBJECT_UNLOCK (bin);
    return NULL;
  }
  appsrc_group = g_malloc0 (sizeof (GstAppSourceGroup));
  appsrc_group->appsrc = gst_element_factory_make ("appsrc", name);

  if (bin->smart_prop)
    g_object_set (appsrc_group->appsrc, "smart-properties", bin->smart_prop,
        NULL);

  bin->appsrc_list = g_list_append (bin->appsrc_list, appsrc_group);
  bin->n_source++;

  GST_INFO_OBJECT (bin, "appsrc %p is appended to a list",
      appsrc_group->appsrc);
  GST_INFO_OBJECT (bin, "source number = %d", bin->n_source);

  GST_OBJECT_UNLOCK (bin);

  return appsrc_group->appsrc;
}

/**
 * gst_dyn_appsrc_end_of_stream:
 * @dynappsrc: a #GstDynAppSrc
 *
 * Indicates to all of appsrc elements that the last buffer queued in the
 * element is the last buffer of the stream.
 *
 * Returns: #GST_FLOW_OK when the EOS was successfuly queued.
 * #GST_FLOW_FLUSHING when @appsrc is not PAUSED or PLAYING.
 */
GstFlowReturn
gst_dyn_appsrc_end_of_stream (GstDynAppSrc * bin)
{
  GstFlowReturn ret = GST_FLOW_OK;
  GList *item;

  for (item = bin->appsrc_list; item; item = g_list_next (item)) {
    GstAppSourceGroup *appsrc_group = (GstAppSourceGroup *) item->data;

    GST_DEBUG_OBJECT (bin, "indicate to appsrc element for EOS");
    g_signal_emit_by_name (appsrc_group->appsrc, "end-of-stream", &ret);
    GST_DEBUG_OBJECT (bin, "%s[ret:%s]",
        GST_ELEMENT_NAME (appsrc_group->appsrc), gst_flow_get_name (ret));
    if (ret != GST_FLOW_OK)
      break;
  }

  return ret;
}

static GstStateChangeReturn
gst_dyn_appsrc_change_state (GstElement * element, GstStateChange transition)
{
  GstStateChangeReturn ret;
  GstDynAppSrc *bin = GST_DYN_APPSRC (element);

  switch (transition) {
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      if (!setup_source (bin))
        return GST_STATE_CHANGE_FAILURE;
      break;
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

  switch (transition) {
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      GST_DEBUG_OBJECT (bin, "ready to paused");
      if (ret == GST_STATE_CHANGE_FAILURE)
        goto setup_failed;
      break;
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      GST_DEBUG_OBJECT (bin, "paused to ready");
      remove_source (bin);
      break;
    case GST_STATE_CHANGE_READY_TO_NULL:
      remove_source (bin);
      break;
    default:
      break;
  }

  return ret;

setup_failed:
  {
    /* clean up leftover groups */
    return GST_STATE_CHANGE_FAILURE;
  }
}

static GstURIType
gst_dyn_appsrc_uri_get_type (GType type)
{
  return GST_URI_SRC;
}

static const gchar *const *
gst_dyn_appsrc_uri_get_protocols (GType type)
{
  static const gchar *protocols[] = { "dynappsrc", NULL };

  return protocols;
}

static gchar *
gst_dyn_appsrc_uri_get_uri (GstURIHandler * handler)
{
  GstDynAppSrc *bin = GST_DYN_APPSRC (handler);

  return bin->uri ? g_strdup (bin->uri) : NULL;
}

static gboolean
gst_dyn_appsrc_uri_set_uri (GstURIHandler * handler, const gchar * uri,
    GError ** error)
{
  return TRUE;
}

static void
gst_dyn_appsrc_uri_handler_init (gpointer g_iface, gpointer iface_data)
{
  GstURIHandlerInterface *iface = (GstURIHandlerInterface *) g_iface;

  iface->get_type = gst_dyn_appsrc_uri_get_type;
  iface->get_protocols = gst_dyn_appsrc_uri_get_protocols;
  iface->get_uri = gst_dyn_appsrc_uri_get_uri;
  iface->set_uri = gst_dyn_appsrc_uri_set_uri;
}
