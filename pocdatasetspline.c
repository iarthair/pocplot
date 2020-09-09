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
#include <glib.h>
#include "pocdatasetspline.h"
#include "pocspline.h"

/**
 * SECTION: pocdatasetspline
 * @title:  PocDatasetSpline
 * @short_description: Dataset for #PocPlot
 * @see_also: #PocPlot #PocAxis #PocDataset
 *
 * A #PocDataset subclass which interpolates a curve from the control points
 * based on the algorithm from Numerical Recipies 2nd Edition.
 * Control points may be highlighted by drawing markers at their locations.
 */

struct _PocDatasetSpline
  {
    PocDataset parent_instance;

    GdkRGBA		marker_stroke;
    GdkRGBA		marker_fill;
    gboolean		show_markers;

    PocPointArray	*points;
    guint		cache_width;
  };

G_DEFINE_TYPE (PocDatasetSpline, poc_dataset_spline, POC_TYPE_DATASET)

/**
 * poc_dataset_spline_new:
 *
 * Create a new #PocDatasetSpline
 *
 * Returns: (transfer full): New #PocDatasetSpline
 */
PocDatasetSpline *
poc_dataset_spline_new (void)
{
  return g_object_new (POC_TYPE_DATASET_SPLINE, NULL);
}

enum
  {
    PROP_0,
    PROP_MARKER_FILL,
    PROP_MARKER_STROKE,
    PROP_SHOW_MARKERS,
    N_PROPERTIES
  };
static GParamSpec *poc_dataset_spline_prop[N_PROPERTIES];

static void poc_dataset_spline_finalize (GObject *object);
static void poc_dataset_spline_get_property (GObject *object, guint param_id,
				     GValue *value, GParamSpec *pspec);
static void poc_dataset_spline_set_property (GObject *object, guint param_id,
				     const GValue *value, GParamSpec *pspec);
static void poc_dataset_spline_draw (PocDataset *dataset, cairo_t *cr,
				     guint width, guint height);
static void poc_dataset_spline_invalidate (PocDataset *dataset);

static void
poc_dataset_spline_class_init (PocDatasetSplineClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  PocDatasetClass *dataset_class = POC_DATASET_CLASS (class);

  gobject_class->finalize = poc_dataset_spline_finalize;
  gobject_class->set_property = poc_dataset_spline_set_property;
  gobject_class->get_property = poc_dataset_spline_get_property;

  dataset_class->draw = poc_dataset_spline_draw;
  dataset_class->invalidate = poc_dataset_spline_invalidate;

  poc_dataset_spline_prop[PROP_MARKER_STROKE] = g_param_spec_boxed (
  	"marker-stroke",
	"Marker Stroke Colour", "Colour for stroking markers",
	GDK_TYPE_RGBA,
	G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  poc_dataset_spline_prop[PROP_MARKER_FILL] = g_param_spec_boxed (
  	"marker-fill",
	"Marker Fill Colour", "Colour for filling markers",
	GDK_TYPE_RGBA,
	G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  poc_dataset_spline_prop[PROP_SHOW_MARKERS] = g_param_spec_boolean (
	"show-markers", "Show Markers", "Show markers on graph lines",
	FALSE,
	G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_properties (gobject_class, N_PROPERTIES, poc_dataset_spline_prop);
}

static void
poc_dataset_spline_init (PocDatasetSpline *self)
{
  GdkRGBA clear = { 0.0, 0.0, 0.0, 0.0 };
  GdkRGBA white = { 1.0, 1.0, 1.0, 1.0 };

  self->marker_stroke = white;
  self->marker_fill = clear;
  self->show_markers = FALSE;
}

static void
poc_dataset_spline_finalize (GObject *object)
{
  PocDatasetSpline *self = (PocDatasetSpline *) object;

  if (self->points != NULL)
    poc_point_array_unref (self->points);
  G_OBJECT_CLASS (poc_dataset_spline_parent_class)->finalize (object);
}

static void
poc_dataset_spline_set_property (GObject *object, guint prop_id,
				 const GValue *value, GParamSpec *pspec)
{
  PocDatasetSpline *self = POC_DATASET_SPLINE (object);

  switch (prop_id)
    {
    case PROP_MARKER_STROKE:
      poc_dataset_spline_set_marker_stroke (self, g_value_get_boxed (value));
      break;
    case PROP_MARKER_FILL:
      poc_dataset_spline_set_marker_fill (self, g_value_get_boxed (value));
      break;
    case PROP_SHOW_MARKERS:
      poc_dataset_spline_set_show_markers (self, g_value_get_boolean (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      return;
    }
}

static void
poc_dataset_spline_get_property (GObject *object, guint prop_id,
				 GValue *value, GParamSpec *pspec)
{
  PocDatasetSpline *self = POC_DATASET_SPLINE (object);
  GdkRGBA rgba;

  switch (prop_id)
    {
    case PROP_MARKER_STROKE:
      poc_dataset_spline_get_marker_stroke (self, &rgba);
      g_value_set_boxed (value, &rgba);
      break;
    case PROP_MARKER_FILL:
      poc_dataset_spline_get_marker_fill (self, &rgba);
      g_value_set_boxed (value, &rgba);
      break;
    case PROP_SHOW_MARKERS:
      g_value_set_boolean (value, poc_dataset_spline_get_show_markers (self));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

/* properties {{{1 */

/* marker stroke {{{2 */

/**
 * poc_dataset_spline_get_marker_stroke:
 * @self: A #PocDatasetSpline
 * @rgba: A #GdkRGBA to receive the marker stroke colour
 *
 * Get the colour for stroking markers.
 */
void
poc_dataset_spline_get_marker_stroke (PocDatasetSpline *self, GdkRGBA *rgba)
{
  g_return_if_fail (POC_IS_DATASET_SPLINE (self));
  g_return_if_fail (rgba != NULL);

  *rgba = self->marker_stroke;
}

/**
 * poc_dataset_spline_set_marker_stroke:
 * @self: A #PocDatasetSpline
 * @rgba: A #GdkRGBA
 *
 * Set the colour for stroking markers.
 */
void
poc_dataset_spline_set_marker_stroke (PocDatasetSpline *self, const GdkRGBA *rgba)
{
  g_return_if_fail (POC_IS_DATASET_SPLINE (self));
  g_return_if_fail (rgba != NULL);

  self->marker_stroke = *rgba;
  poc_dataset_notify_update (POC_DATASET (self));
  g_object_notify_by_pspec (G_OBJECT (self), poc_dataset_spline_prop[PROP_MARKER_STROKE]);
}

/* marker fill {{{2 */

/**
 * poc_dataset_spline_get_marker_fill:
 * @self: A #PocDatasetSpline
 * @rgba: A #GdkRGBA
 *
 * Set the colour for stroking markers.
 */
void
poc_dataset_spline_get_marker_fill (PocDatasetSpline *self, GdkRGBA *rgba)
{
  g_return_if_fail (POC_IS_DATASET_SPLINE (self));
  g_return_if_fail (rgba != NULL);

  *rgba = self->marker_fill;
}

/**
 * poc_dataset_spline_set_marker_fill:
 * @self: A #PocDatasetSpline
 * @rgba: A #GdkRGBA to receive the marker stroke colour
 *
 * Get the colour for stroking markers.
 */
void
poc_dataset_spline_set_marker_fill (PocDatasetSpline *self, const GdkRGBA *rgba)
{
  g_return_if_fail (POC_IS_DATASET_SPLINE (self));
  g_return_if_fail (rgba != NULL);

  self->marker_fill = *rgba;
  poc_dataset_notify_update (POC_DATASET (self));
  g_object_notify_by_pspec (G_OBJECT (self), poc_dataset_spline_prop[PROP_MARKER_FILL]);
}

/* show markers {{{2 */

/**
 * poc_dataset_spline_set_show_markers:
 * @self: A #PocDatasetSpline
 * @value: Show markers if %TRUE
 *
 * Set whether to show markers on control points.
 */
void
poc_dataset_spline_set_show_markers (PocDatasetSpline *self, gboolean value)
{
  g_return_if_fail (POC_IS_DATASET_SPLINE (self));
  self->show_markers = !!value;
  poc_dataset_notify_update (POC_DATASET (self));
  g_object_notify_by_pspec (G_OBJECT (self), poc_dataset_spline_prop[PROP_SHOW_MARKERS]);
}

/**
 * poc_dataset_spline_get_show_markers:
 * @self: A #PocDatasetSpline
 *
 * Get whether to show markers on control points.
 *
 * Returns: %TRUE if markers are shown.
 */
gboolean
poc_dataset_spline_get_show_markers (PocDatasetSpline *self)
{
  g_return_val_if_fail (POC_IS_DATASET_SPLINE (self), FALSE);
  return self->show_markers;
}

/* override class methods {{{1 */

static void
poc_dataset_spline_invalidate (PocDataset *dataset)
{
  PocDatasetSpline *self = POC_DATASET_SPLINE (dataset);

  POC_DATASET_CLASS (poc_dataset_spline_parent_class)->invalidate (dataset);
  if (self->points != NULL)
    {
      poc_point_array_unref (self->points);
      self->points = NULL;
    }
}

static void
poc_dataset_spline_draw (PocDataset *dataset, cairo_t *cr,
			 guint width, guint height)
{
  PocDatasetSpline *self = POC_DATASET_SPLINE (dataset);
  PocPointArray *spline;
  gdouble min_x, max_x;
  PocPoint p;
  guint i;
  PocAxis *x_axis, *y_axis;
  GdkRGBA line_stroke;
  PocLineStyle line_style;
  const double *dashes;
  int num_dashes;

  spline = poc_dataset_get_points (POC_DATASET (self));
  if (spline == NULL)
    return;

  x_axis = poc_dataset_get_x_axis (dataset);
  y_axis = poc_dataset_get_y_axis (dataset);
  poc_dataset_get_line_stroke (dataset, &line_stroke);
  line_style = poc_dataset_get_line_style (dataset);

  if (self->points == NULL || self->cache_width != width)
    {
      self->cache_width = width;
      poc_axis_get_display_range (x_axis, &min_x, &max_x);
      if (self->points != NULL)
	poc_point_array_unref (self->points);
      self->points = poc_spline_get_points (spline, min_x, max_x,
      					    width / 4 + 1);
    }

  /* Draw the plot line */
  cairo_new_path (cr);
  p = poc_point_array_index (self->points, 0);
  cairo_move_to (cr, poc_axis_project (x_axis, p.x, width),
		     poc_axis_project (y_axis, p.y, -height));
  for (i = 1; i < poc_point_array_len (self->points); i++)
    {
      p = poc_point_array_index (self->points, i);
      cairo_line_to (cr, poc_axis_project (x_axis, p.x, width),
			 poc_axis_project (y_axis, p.y, -height));
    }

  /* Stroke the line */
  //TODO Line width
  cairo_set_line_width (cr, 1.0);
  dashes = poc_line_style_get_dashes (line_style, &num_dashes);
  cairo_set_dash (cr, dashes, num_dashes, 0.0);
  gdk_cairo_set_source_rgba (cr, &line_stroke);
  cairo_stroke (cr);

  if (self->show_markers)
    {
      cairo_new_path (cr);
      for (i = 0; i < poc_point_array_len (spline); i++)
	{
	  cairo_new_sub_path (cr);
	  //TODO Marker size and style properties
	  p = poc_point_array_index (spline, i);
	  cairo_arc (cr, poc_axis_project (x_axis, p.x, width),
			 poc_axis_project (y_axis, p.y, -height),
		     3, 0.0, 2.0 * G_PI);
	}
      gdk_cairo_set_source_rgba (cr, &self->marker_fill);
      cairo_fill_preserve (cr);
      gdk_cairo_set_source_rgba (cr, &self->marker_stroke);
      cairo_stroke (cr);
    }
}
