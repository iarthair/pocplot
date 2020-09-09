/* This file is part of PocPlot.
 *
 * PocPlot is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * PocPlot is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with PocPlot; if not, see
 * <https://www.gnu.org/licenses/>.
 *
 * Copyright 2020 Brian Stafford
 */
/*
 * PocSample
 */

#include "pocsample.h"
#include <math.h>

typedef struct _PocSample PocSample;
struct _PocSample
  {
    GtkDrawingArea	parent_instance;

    PocDataset *	dataset;
  };

G_DEFINE_TYPE (PocSample, poc_sample, GTK_TYPE_DRAWING_AREA)

/**
 * SECTION: pocsample
 * @title:  PocSample
 * @short_description: Line sample widget for use with #PocDataset.
 * @see_also: #PocDataset
 *
 * Generate a sample of the line used to plot data in the associated
 * #PocDataset.  This is useful for use as an "image" in buttons or labels
 * elsewhere in the UI.
 */

/**
 * poc_sample_new:
 * @dataset: a #PocDataset
 *
 * Create a new #PocSample showing a line sample for @dataset.
 *
 * Returns: (transfer full): New #PocSample
 */
PocSample *
poc_sample_new (PocDataset *dataset)
{
  return g_object_new (POC_TYPE_SAMPLE, "dataset", dataset, NULL);
}

enum
  {
    PROP_0,
    PROP_DATASET,
    N_PROPERTIES
  };
static GParamSpec *poc_sample_prop[N_PROPERTIES];

static void poc_sample_dispose (GObject *object);
static void poc_sample_finalize (GObject *object);
static void poc_sample_get_property (GObject *object, guint param_id,
                                     GValue *value, GParamSpec *pspec);
static void poc_sample_set_property (GObject *object, guint param_id,
                                     const GValue *value, GParamSpec *pspec);
static gboolean poc_sample_draw (GtkWidget *widget, cairo_t *cr);

/* GObject {{{1 */

static void
poc_sample_class_init (PocSampleClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);

  gobject_class->dispose = poc_sample_dispose;
  gobject_class->finalize = poc_sample_finalize;
  gobject_class->set_property = poc_sample_set_property;
  gobject_class->get_property = poc_sample_get_property;

  gtk_widget_class_set_css_name (widget_class, "sample");
  widget_class->draw = poc_sample_draw;

  poc_sample_prop[PROP_DATASET] = g_param_spec_object (
        "dataset", "Dataset",
        "Generate a line sample for the referenced dataset.",
        POC_TYPE_DATASET,
        G_PARAM_CONSTRUCT | G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (gobject_class, N_PROPERTIES, poc_sample_prop);
}

static void
poc_sample_init (PocSample *self)
{
  self->dataset = NULL;
}

static void
poc_sample_dispose (GObject *object)
{
  PocSample *self = (PocSample *) object;

  g_clear_object (&self->dataset);
  G_OBJECT_CLASS (poc_sample_parent_class)->dispose (object);
}

static void
poc_sample_finalize (GObject *object)
{
  //PocSample *self = (PocSample *) object;

  G_OBJECT_CLASS (poc_sample_parent_class)->finalize (object);
}

/* Properties {{{1 */

static void
poc_sample_set_property (GObject *object, guint prop_id,
                         const GValue *value, GParamSpec *pspec)
{
  PocSample *self = POC_SAMPLE (object);

  switch (prop_id)
    {
    case PROP_DATASET:
      poc_sample_set_dataset (self, g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      return;
    }
}


static void
poc_sample_get_property (GObject *object, guint prop_id,
                         GValue *value, GParamSpec *pspec)
{
  PocSample *self = POC_SAMPLE (object);

  switch (prop_id)
    {
    case PROP_DATASET:
      g_value_set_object (value, poc_sample_get_dataset (self));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

/* "dataset" {{{2 */

/**
 * poc_sample_set_dataset:
 * @self: A #PocSample
 * @dataset: A #PocDataset
 *
 * Display a line sample for the associated dataset widget.
 */
void
poc_sample_set_dataset (PocSample *self, PocDataset *dataset)
{
  g_return_if_fail (POC_IS_SAMPLE (self));

  if (g_set_object (&self->dataset, dataset))
    g_object_notify_by_pspec (G_OBJECT (self), poc_sample_prop[PROP_DATASET]);
}

/**
 * poc_sample_get_dataset:
 * @self: A #PocSample
 *
 * Return the associated dataset.
 *
 * Returns: (transfer none): a #PocDataset
 */
PocDataset *
poc_sample_get_dataset (PocSample *self)
{
  g_return_val_if_fail (POC_IS_SAMPLE (self), NULL);

  return self->dataset;
}

/* done */

static gboolean
poc_sample_draw (GtkWidget *widget, cairo_t *cr)
{
  PocSample *self = (PocSample *) widget;
  GdkRGBA rgba;
  const double *dashes;
  int num_dashes;
  PocLineStyle line_style;
  GtkAllocation allocation;
  GtkStyleContext *style;

  if (self->dataset == NULL)
    return FALSE;

  style = gtk_widget_get_style_context (GTK_WIDGET (self));

  /* fetch sample parameters from dataset */
  poc_dataset_get_line_stroke (self->dataset, &rgba);
  line_style = poc_dataset_get_line_style (self->dataset);

  gtk_widget_get_allocation (widget, &allocation);
  gtk_render_background (style, cr, 0, 0, allocation.width, allocation.height);

  cairo_move_to (cr, allocation.width / 10.0, round (allocation.height / 2.0) + 0.5);
  cairo_rel_line_to (cr, allocation.width - allocation.width / 5.0, 0.0);

  /* stroke the sample line */
  cairo_set_line_width (cr, 1.0);
  dashes = poc_line_style_get_dashes (line_style, &num_dashes);
  cairo_set_dash (cr, dashes, num_dashes, 0.0);
  gdk_cairo_set_source_rgba (cr, &rgba);
  cairo_stroke (cr);
  return FALSE;
}
