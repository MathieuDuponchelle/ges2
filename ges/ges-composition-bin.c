#include "ges-composition-bin.h"
#include "nle.h"

/* Structure definitions */

typedef struct _GESCompositionBinPrivate
{
  GList *nleobjects;
  GList *ghostpads;
  guint expected_async_done;
  guint group_id;
} GESCompositionBinPrivate;

struct _GESCompositionBin
{
  GstBin parent;
  GESCompositionBinPrivate *priv;
};

#define NLE_OBJECT_OLD_PARENT (g_quark_from_string ("nle_object_old_parent_quark"))

G_DEFINE_TYPE (GESCompositionBin, ges_composition_bin, GST_TYPE_BIN)

static GstPadProbeReturn
_pad_probe_cb (GstPad * mixer_pad, GstPadProbeInfo * info,
    GESCompositionBin * self)
{
  GstEvent *event = GST_PAD_PROBE_INFO_EVENT (info);
  if (GST_EVENT_TYPE (event) == GST_EVENT_STREAM_START) {
    if (self->priv->group_id == -1) {
      if (!gst_event_parse_group_id (event, &self->priv->group_id))
        self->priv->group_id = gst_util_group_id_next ();
    }

    info->data = gst_event_make_writable (event);
    gst_event_set_group_id (GST_PAD_PROBE_INFO_EVENT (info),
        self->priv->group_id);

    return GST_PAD_PROBE_REMOVE;
  }

  return GST_PAD_PROBE_OK;
}

static void
_expose_nle_objects (GESCompositionBin *self)
{
  GList *tmp, *gpads;

  gpads = self->priv->ghostpads;

  for (tmp = self->priv->nleobjects; tmp; tmp = tmp->next) {
    GstElement *capsfilter = gst_element_factory_make ("capsfilter", NULL);
    GstCaps *caps;
    GstElement *old_parent;
    GstPad *pad = gst_element_get_static_pad (capsfilter, "src");

    g_object_get (tmp->data, "composition", &old_parent, NULL);

    if (old_parent) {
      gst_object_ref (tmp->data);
      gst_bin_remove (GST_BIN (old_parent), GST_ELEMENT (tmp->data));
      g_object_set_qdata (tmp->data, NLE_OBJECT_OLD_PARENT, gst_object_ref (old_parent));
      nle_object_commit (NLE_OBJECT (old_parent), TRUE);
    }

    g_object_get (tmp->data, "caps", &caps, NULL);
    g_object_set (capsfilter, "caps", gst_caps_copy (caps), NULL);
    gst_caps_unref (caps);

    gst_bin_add_many (GST_BIN (self), capsfilter, tmp->data, NULL);
    gst_element_link (tmp->data, capsfilter);

    gst_ghost_pad_set_target (GST_GHOST_PAD (gpads->data), pad);
    gst_pad_set_active (GST_PAD(gpads->data), TRUE);

    gst_pad_add_probe (pad,
        GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM,
        (GstPadProbeCallback) _pad_probe_cb, self, NULL);

    gpads = gpads->next;
  }

  gst_element_no_more_pads (GST_ELEMENT (self));
}

static gboolean
_remove_child (GValue * item, GValue * ret G_GNUC_UNUSED, GstBin * bin)
{
  GstElement *child = g_value_get_object (item);

  gst_bin_remove (bin, child);

  return TRUE;
}

static void
_empty_bin (GstBin * bin)
{
  GstIterator *children;

  children = gst_bin_iterate_elements (bin);

  while (G_UNLIKELY (gst_iterator_fold (children,
              (GstIteratorFoldFunction) _remove_child, NULL,
              bin) == GST_ITERATOR_RESYNC)) {
    gst_iterator_resync (children);
  }

  gst_iterator_free (children);
}

static void
_unexpose_nle_objects (GESCompositionBin *self)
{
  GList *tmp;
  GList *gpads = self->priv->ghostpads;

  g_list_foreach (self->priv->nleobjects, (GFunc) gst_object_ref, NULL);
  _empty_bin (GST_BIN (self));

  for (tmp = self->priv->nleobjects; tmp; tmp = tmp->next) {
    GstElement *old_parent;

    gst_ghost_pad_set_target (GST_GHOST_PAD (gpads->data), NULL);
    gst_pad_set_active (GST_PAD (gpads->data), FALSE);
    old_parent = GST_ELEMENT (g_object_get_qdata (tmp->data, NLE_OBJECT_OLD_PARENT));

    if (old_parent) {
      gst_bin_add (GST_BIN (old_parent), GST_ELEMENT (tmp->data));
      gst_object_unref (old_parent);
      nle_object_commit (NLE_OBJECT (old_parent), TRUE);
      g_object_set_qdata (tmp->data, NLE_OBJECT_OLD_PARENT, NULL);
    }

    gpads = gpads->next;
  }
}

/* Implementation */

static void
ges_composition_bin_handle_message (GstBin * bin, GstMessage * message)
{
  GESCompositionBin *self = GES_COMPOSITION_BIN (bin);

  if (GST_MESSAGE_TYPE (message) == GST_MESSAGE_ELEMENT) {
    GstMessage *amessage = NULL;
    const GstStructure *mstructure = gst_message_get_structure (message);

    if (gst_structure_has_name (mstructure, "NleCompositionStartUpdate")) {
      if (g_strcmp0 (gst_structure_get_string (mstructure, "reason"), "Seek")) {
        GST_DEBUG_OBJECT (self,
            "A composition is starting an update because of %s"
            " not concidering async", gst_structure_get_string (mstructure,
                "reason"));

        goto forward;
      }

      GST_OBJECT_LOCK (self);
      if (self->priv->expected_async_done == 0) {
        amessage = gst_message_new_async_start (GST_OBJECT_CAST (bin));
        self->priv->expected_async_done = g_list_length (self->priv->nleobjects);
        GST_INFO_OBJECT (self, "Posting ASYNC_START %s",
            gst_structure_get_string (mstructure, "reason"));
      }
      GST_OBJECT_UNLOCK (self);

    } else if (gst_structure_has_name (mstructure, "NleCompositionUpdateDone")) {
      if (g_strcmp0 (gst_structure_get_string (mstructure, "reason"), "Seek")) {
        GST_DEBUG_OBJECT (self,
            "A composition is done updating because of %s"
            " not concidering async", gst_structure_get_string (mstructure,
                "reason"));

        goto forward;
      }

      GST_OBJECT_LOCK (self);
      self->priv->expected_async_done -= 1;
      GST_DEBUG ("expected async done is : %d", self->priv->expected_async_done);
      if (self->priv->expected_async_done == 0) {
        amessage = gst_message_new_async_done (GST_OBJECT_CAST (bin),
            GST_CLOCK_TIME_NONE);
        GST_DEBUG_OBJECT (self, "Posting ASYNC_DONE %s",
            gst_structure_get_string (mstructure, "reason"));
      }
      GST_OBJECT_UNLOCK (self);
    }

    if (amessage)
      gst_element_post_message (GST_ELEMENT_CAST (bin), amessage);
  }

forward:
  gst_element_post_message (GST_ELEMENT_CAST (bin), message);
}

void
_create_ghostpads (GESCompositionBin *self)
{
  guint i;
  GstPad *ghostpad;

  if (self->priv->ghostpads)
    return;

  for (i = g_list_length (self->priv->nleobjects); i > 0; i --) {
    ghostpad = gst_ghost_pad_new_no_target (NULL, GST_PAD_SRC);
    gst_element_add_pad (GST_ELEMENT (self), ghostpad);
    self->priv->ghostpads = g_list_append (self->priv->ghostpads, ghostpad);
  }
}

/* API */

void
ges_composition_bin_set_nle_objects (GESCompositionBin *self, GList *nleobjects)
{
  _unexpose_nle_objects (self);
  self->priv->nleobjects = nleobjects;
  _create_ghostpads (self);
  _expose_nle_objects (self);
}

GESCompositionBin *
ges_composition_bin_new (void)
{
  return g_object_new (GES_TYPE_COMPOSITION_BIN, NULL);
}

/* GObject initialization */

static void
ges_composition_bin_class_init (GESCompositionBinClass *klass)
{
  GstBinClass *bin_class = GST_BIN_CLASS (klass);

  g_type_class_add_private (klass, sizeof (GESCompositionBinPrivate));

  bin_class->handle_message = GST_DEBUG_FUNCPTR (ges_composition_bin_handle_message);
}

static void
ges_composition_bin_init (GESCompositionBin *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
      GES_TYPE_COMPOSITION_BIN, GESCompositionBinPrivate);

  self->priv->nleobjects = NULL;
  self->priv->ghostpads = NULL;
  self->priv->expected_async_done = 0;
  self->priv->group_id = -1;
}
