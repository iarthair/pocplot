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
#include "pocdataset.h"
#include "pocplot.h"

/**
 * SECTION: pocdataset
 * @title:  PocDataset
 * @short_description: Dataset for #PocPlot
 * @see_also: #PocPlot #PocAxis #PocDatasetSpline
 *
 * A #PocDataset represents data to be plotted on a #PocPlot canvas.  Any
 * number of datasets may be added to a plot.  Each dataset has X and Y axes
 * which controls the range of data plotted and allow optionally allow the plot
 * to be scrolled.
 *
 * Although the dataset contains drawing code, drawing always takes place under
 * control of the #PocPlot and on its canvas.
 *
 * The #PocDataset base class draws a plot using straight line segments between
 * each of the control points, however it may be subclassed for alternative
 * plotting algorithms.
 */

typedef struct _PocDatasetPrivate PocDatasetPrivate;
struct _PocDatasetPrivate
  {
    PocPointArray	*points;
    gchar 		*nickname;
    gchar 		*legend;
    GdkRGBA		line_stroke;
    PocLineStyle	line_style;
    PocAxis		*x_axis;
    PocAxis		*y_axis;
  };

G_DEFINE_TYPE_WITH_PRIVATE (PocDataset, poc_dataset, G_TYPE_OBJECT)

/**
 * poc_dataset_new:
 *
 * Create a new #PocDataset
 *
 * Returns: (transfer full): New #PocDataset
 */
PocDataset *
poc_dataset_new (void)
{
  return g_object_new (POC_TYPE_DATASET, NULL);
}

/* GObject {{{1 */

enum
  {
    PROP_0,
    PROP_POINTS,

    PROP_NICKNAME,
    PROP_LEGEND,

    PROP_LINE_STROKE,
    PROP_LINE_STYLE,

    PROP_X_AXIS,
    PROP_Y_AXIS,

    N_PROPERTIES
  };
static GParamSpec *poc_dataset_prop[N_PROPERTIES];

enum
  {
    UPDATE,
    N_SIGNAL
  };
static guint poc_dataset_signals[N_SIGNAL];

static void poc_dataset_dispose (GObject *object);
static void poc_dataset_finalize (GObject *object);
static void poc_dataset_get_property (GObject *object, guint param_id,
				     GValue *value, GParamSpec *pspec);
static void poc_dataset_set_property (GObject *object, guint param_id,
				     const GValue *value, GParamSpec *pspec);
static void poc_dataset_draw_real (PocDataset *self, cairo_t *cr,
		       		   guint width, guint height);
static void poc_dataset_invalidate_real (PocDataset *self);

static void
poc_dataset_class_init (PocDatasetClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);

  gobject_class->dispose = poc_dataset_dispose;
  gobject_class->finalize = poc_dataset_finalize;
  gobject_class->set_property = poc_dataset_set_property;
  gobject_class->get_property = poc_dataset_get_property;

  class->draw = poc_dataset_draw_real;
  class->invalidate = poc_dataset_invalidate_real;

  poc_dataset_prop[PROP_POINTS] = g_param_spec_boxed (
	"points", "Points", "Array of (X,Y) coordinates",
	POC_TYPE_POINT_ARRAY,
	G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  poc_dataset_prop[PROP_NICKNAME] = g_param_spec_string (
  	"nickname",
	"Nickname",
	"Nickname for the dataset (\"nickname\" since Glade doesn't like \"name\")",
	NULL,
	G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  poc_dataset_prop[PROP_LINE_STROKE] = g_param_spec_boxed (
  	"line-stroke",
	"Line Stroke Colour", "Colour for stroking lines",
	GDK_TYPE_RGBA,
	G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  poc_dataset_prop[PROP_LINE_STYLE] = g_param_spec_enum (
        "line-style", "Line Style",
        "Line style for plot line",
        POC_TYPE_LINE_STYLE, POC_LINE_STYLE_SOLID,
        G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  poc_dataset_prop[PROP_LEGEND] = g_param_spec_string (
  	"legend",
	"Legend", "Legend for the item",
	NULL,
	G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  poc_dataset_prop[PROP_X_AXIS] = g_param_spec_object (
  	"x-axis",
	"X-Axis", "X axis for dataset",
	POC_TYPE_AXIS,
	G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  poc_dataset_prop[PROP_Y_AXIS] = g_param_spec_object (
  	"y-axis",
	"Y-Axis", "Y axis for dataset",
	POC_TYPE_AXIS,
	G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (gobject_class, N_PROPERTIES, poc_dataset_prop);

  poc_dataset_signals[UPDATE] = g_signal_new (
	"update", G_TYPE_FROM_CLASS (class),
	G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
	0, NULL, NULL,
	g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
}


static void
poc_dataset_init (PocDataset *self)
{
  PocDatasetPrivate *priv = poc_dataset_get_instance_private (self);

  GdkRGBA white = { 1.0, 1.0, 1.0, 1.0 };

  priv->line_stroke = white;
  priv->line_style = POC_LINE_STYLE_SOLID;
}

static void
poc_dataset_dispose (GObject *object)
{
  PocDataset *self = (PocDataset *) object;
  PocDatasetPrivate *priv = poc_dataset_get_instance_private (self);

  g_clear_object (&priv->x_axis);
  g_clear_object (&priv->y_axis);

  G_OBJECT_CLASS (poc_dataset_parent_class)->dispose (object);
}

static void
poc_dataset_finalize (GObject *object)
{
  PocDataset *self = (PocDataset *) object;
  PocDatasetPrivate *priv = poc_dataset_get_instance_private (self);

  if (priv->points != NULL)
    poc_point_array_unref (priv->points);
  g_free (priv->nickname);
  g_free (priv->legend);
  G_OBJECT_CLASS (poc_dataset_parent_class)->finalize (object);
}

static void
poc_dataset_set_property (GObject *object, guint prop_id,
			 const GValue *value, GParamSpec *pspec)
{
  PocDataset *self = POC_DATASET (object);

  switch (prop_id)
    {
    case PROP_LINE_STYLE:
      poc_dataset_set_line_style (self, g_value_get_enum (value));
      break;
    case PROP_POINTS:
      poc_dataset_set_points (self, g_value_get_boxed (value));
      break;
    case PROP_NICKNAME:
      poc_dataset_set_nickname (self, g_value_get_string (value));
      break;
    case PROP_LEGEND:
      poc_dataset_set_legend (self, g_value_get_string (value));
      break;

    case PROP_LINE_STROKE:
      poc_dataset_set_line_stroke (self, g_value_get_boxed (value));
      break;

    case PROP_X_AXIS:
      poc_dataset_set_x_axis (self, g_value_get_object (value));
      break;
    case PROP_Y_AXIS:
      poc_dataset_set_y_axis (self, g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      return;
    }
}

static void
poc_dataset_get_property (GObject *object, guint prop_id,
			 GValue *value, GParamSpec *pspec)
{
  PocDataset *self = POC_DATASET (object);
  GdkRGBA rgba;

  switch (prop_id)
    {
    case PROP_LINE_STYLE:
      g_value_set_enum (value, poc_dataset_get_line_style (self));
      break;
    case PROP_POINTS:
      g_value_set_boxed (value, poc_dataset_get_points (self));
      break;
    case PROP_NICKNAME:
      g_value_set_string (value, poc_dataset_get_nickname (self));
      break;
    case PROP_LEGEND:
      g_value_set_string (value, poc_dataset_get_legend (self));
      break;

    case PROP_LINE_STROKE:
      poc_dataset_get_line_stroke (self, &rgba);
      g_value_set_boxed (value, &rgba);
      break;

    case PROP_X_AXIS:
      g_value_set_object (value, poc_dataset_get_x_axis (self));
      break;
    case PROP_Y_AXIS:
      g_value_set_object (value, poc_dataset_get_y_axis (self));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

/* Properties {{{1 */

/* nickname {{{2 */

/**
 * poc_dataset_set_nickname:
 * @self: A #PocDataset
 * @nickname: dataset name
 *
 * Set the name for the dataset.
 */
void
poc_dataset_set_nickname (PocDataset *self, const gchar *nickname)
{
  PocDatasetPrivate *priv = poc_dataset_get_instance_private (self);

  g_return_if_fail (POC_IS_DATASET (self));

  g_free (priv->nickname);
  priv->nickname = g_strdup (nickname);
  g_object_notify_by_pspec (G_OBJECT (self), poc_dataset_prop[PROP_NICKNAME]);
}

/**
 * poc_dataset_get_nickname:
 * @self: A #PocDataset
 *
 * Get the name for the dataset.
 *
 * Returns: (transfer none): the nickname
 */
const gchar *
poc_dataset_get_nickname (PocDataset *self)
{
  PocDatasetPrivate *priv = poc_dataset_get_instance_private (self);

  g_return_val_if_fail (POC_IS_DATASET (self), NULL);
  return priv->nickname;
}

/* legend {{{2 */

/**
 * poc_dataset_set_legend:
 * @self: A #PocDataset
 * @legend: legend text
 *
 * Set the legend for the dataset item.
 */
void
poc_dataset_set_legend (PocDataset *self, const gchar *legend)
{
  PocDatasetPrivate *priv = poc_dataset_get_instance_private (self);

  g_return_if_fail (POC_IS_DATASET (self));

  g_free (priv->legend);
  priv->legend = g_strdup (legend);
  g_object_notify_by_pspec (G_OBJECT (self), poc_dataset_prop[PROP_LEGEND]);
}

/**
 * poc_dataset_get_legend:
 * @self: A #PocDataset
 *
 * Get the legend for the dataset item.
 *
 * Returns: (transfer none): legend text
 */
const gchar *
poc_dataset_get_legend (PocDataset *self)
{
  PocDatasetPrivate *priv = poc_dataset_get_instance_private (self);

  g_return_val_if_fail (POC_IS_DATASET (self), NULL);

  return priv->legend;
}

/* line stroke {{{2 */

/**
 * poc_dataset_get_line_stroke:
 * @self: A #PocDataset
 * @rgba: A #GdkRGBA to receive the line stroke colour
 *
 * Get the colour for stroking plot lines.
 */
void
poc_dataset_get_line_stroke (PocDataset *self, GdkRGBA *rgba)
{
  PocDatasetPrivate *priv = poc_dataset_get_instance_private (self);

  g_return_if_fail (POC_IS_DATASET (self));
  g_return_if_fail (rgba != NULL);

  *rgba = priv->line_stroke;
}

/**
 * poc_dataset_set_line_stroke:
 * @self: A #PocDataset
 * @rgba: A #GdkRGBA
 *
 * Set the colour for stroking plot lines.
 */
void
poc_dataset_set_line_stroke (PocDataset *self, const GdkRGBA *rgba)
{
  PocDatasetPrivate *priv = poc_dataset_get_instance_private (self);

  g_return_if_fail (POC_IS_DATASET (self));
  g_return_if_fail (rgba != NULL);

  priv->line_stroke = *rgba;
  poc_dataset_notify_update (self);
  g_object_notify_by_pspec (G_OBJECT (self), poc_dataset_prop[PROP_LINE_STROKE]);
}

/* "line-style" {{{2 */

/**
 * poc_dataset_set_line_style:
 * @self: A #PocDataset
 * @line_style:  Requested line style
 *
 * Set the line style for stroking dataset plot lines.
 */
void
poc_dataset_set_line_style (PocDataset *self, PocLineStyle line_style)
{
  PocDatasetPrivate *priv = poc_dataset_get_instance_private (self);

  g_return_if_fail (POC_IS_DATASET (self));

  if (priv->line_style != line_style)
    {
      priv->line_style = line_style;
      poc_dataset_notify_update (self);
      g_object_notify_by_pspec (G_OBJECT (self), poc_dataset_prop[PROP_LINE_STYLE]);
    }
}

/**
 * poc_dataset_get_line_style:
 * @self: A #PocDataset
 *
 * Get the dataset's line style
 *
 * Returns: a #PocLineStyle
 */
PocLineStyle
poc_dataset_get_line_style (PocDataset *self)
{
  PocDatasetPrivate *priv = poc_dataset_get_instance_private (self);

  g_return_val_if_fail (POC_IS_DATASET (self), POC_LINE_STYLE_SOLID);

  return priv->line_style;
}

/* X Axis {{{2 */

/**
 * poc_dataset_set_x_axis:
 * @self: A #PocDataset
 * @axis: a #PocAxis
 *
 * Set the X axis associated with this dataset.
 */
void
poc_dataset_set_x_axis (PocDataset *self, PocAxis *axis)
{
  PocDatasetPrivate *priv = poc_dataset_get_instance_private (self);

  g_return_if_fail (POC_IS_DATASET (self));
  g_return_if_fail (POC_IS_AXIS (axis));

  g_set_object (&priv->x_axis, axis);
  poc_dataset_notify_update (self);
  g_object_notify_by_pspec (G_OBJECT (self), poc_dataset_prop[PROP_X_AXIS]);
}

/**
 * poc_dataset_get_x_axis:
 * @self: A #PocDataset
 *
 * Get the X axis associated with this dataset.
 *
 * Returns: (transfer none): a #PocAxis
 */
PocAxis *
poc_dataset_get_x_axis (PocDataset *self)
{
  PocDatasetPrivate *priv = poc_dataset_get_instance_private (self);

  g_return_val_if_fail (POC_IS_DATASET (self), NULL);

  return priv->x_axis;
}

/* Y Axis {{{2 */

/**
 * poc_dataset_set_y_axis:
 * @self: A #PocDataset
 * @axis: a #PocAxis
 *
 * Set the Y axis associated with this dataset.
 */
void
poc_dataset_set_y_axis (PocDataset *self, PocAxis *axis)
{
  PocDatasetPrivate *priv = poc_dataset_get_instance_private (self);

  g_return_if_fail (POC_IS_DATASET (self));
  g_return_if_fail (POC_IS_AXIS (axis));

  g_set_object (&priv->y_axis, axis);
  poc_dataset_notify_update (self);
  g_object_notify_by_pspec (G_OBJECT (self), poc_dataset_prop[PROP_Y_AXIS]);
}

/**
 * poc_dataset_get_y_axis:
 * @self: A #PocDataset
 *
 * Get the X axis associated with this dataset.
 *
 * Returns: (transfer none): a #PocAxis
 */
PocAxis *
poc_dataset_get_y_axis (PocDataset *self)
{
  PocDatasetPrivate *priv = poc_dataset_get_instance_private (self);

  g_return_val_if_fail (POC_IS_DATASET (self), NULL);

  return priv->y_axis;
}

/* points {{{2 */

/**
 * poc_dataset_set_points:
 * @self: A #PocDataset
 * @points: A #PocPointArray with the control points
 *
 * Set the array of control points for the dataset.
 */
void
poc_dataset_set_points (PocDataset *self, PocPointArray *points)
{
  PocDatasetPrivate *priv = poc_dataset_get_instance_private (self);
  PocPointArray *old;

  g_return_if_fail (POC_IS_DATASET (self));

  old = priv->points;
  priv->points = points != NULL ? poc_point_array_ref (points) : NULL;
  if (old != NULL)
    poc_point_array_unref (old);
  poc_dataset_invalidate (self);
  poc_dataset_notify_update (self);
  g_object_notify_by_pspec (G_OBJECT (self), poc_dataset_prop[PROP_POINTS]);
}

/**
 * poc_dataset_get_points:
 * @self: A #PocDataset
 *
 * Get the array of control points for the dataset.
 *
 * Returns: (transfer none): A #PocPointArray
 */
PocPointArray *
poc_dataset_get_points (PocDataset *self)
{
  PocDatasetPrivate *priv = poc_dataset_get_instance_private (self);

  g_return_val_if_fail (POC_IS_DATASET (self), NULL);

  return priv->points;
}

/**
 * poc_dataset_set_points_array:
 * @self: A #PocDataset
 * @x: (array length=points): Array of x coordinates
 * @y: (array length=points): Array of y coordinates
 * @points: number of points in coordinate arrays
 *
 * Set the array of control points for the dataset. X and Y coordinates
 * for each control point are specified in two arrays.
 */
void
poc_dataset_set_points_array (PocDataset *self,
			      const gdouble *x, const gdouble *y, guint points)
{
  PocDatasetPrivate *priv = poc_dataset_get_instance_private (self);
  PocPoint p;
  guint i;

  g_return_if_fail (POC_IS_DATASET (self));

  priv->points = poc_point_array_sized_new (points);
  for (i = 0; i < points; i++)
    {
      p.x = x[i];
      p.y = y[i];
      poc_point_array_append_val (priv->points, p);
    }
}

/* virtual/private methods {{{1 */

void
poc_dataset_notify_update (PocDataset *self)
{
  g_return_if_fail (POC_IS_DATASET (self));
  g_signal_emit (self, poc_dataset_signals[UPDATE], 0);
}

/**
 * poc_dataset_invalidate:
 * @self: A #PocDataset
 *
 * Notify subclasses cached data is invalid.
 */
void
poc_dataset_invalidate (PocDataset *self)
{
  PocDatasetClass *class;

  g_return_if_fail (POC_IS_DATASET (self));

  class = POC_DATASET_GET_CLASS (self);
  g_return_if_fail (class->invalidate != NULL);
  (*class->invalidate) (self);
}

static void
poc_dataset_invalidate_real (G_GNUC_UNUSED PocDataset *self)
{
}

/**
 * poc_dataset_draw:
 * @self: A #PocDataset
 * @cr: A #cairo_t
 * @width: Plot area width
 * @height: Plot area height
 *
 * Draw the dataset in the main plot area.  Used by #PocPlot.
 */
void
poc_dataset_draw (PocDataset *self, cairo_t *cr, guint width, guint height)
{
  PocDatasetPrivate *priv = poc_dataset_get_instance_private (self);
  PocDatasetClass *class;

  g_return_if_fail (POC_IS_DATASET (self));
  g_return_if_fail (POC_IS_AXIS (priv->x_axis));
  g_return_if_fail (POC_IS_AXIS (priv->y_axis));

  class = POC_DATASET_GET_CLASS (self);
  g_return_if_fail (class->draw != NULL);
  (*class->draw) (self, cr, width, height);
}

static void
poc_dataset_draw_real (PocDataset *self, cairo_t *cr,
		       guint width, guint height)
{
  PocDatasetPrivate *priv = poc_dataset_get_instance_private (self);
  PocPoint p;
  const double *dashes;
  int num_dashes;
  guint i;

  if (priv->points == NULL)
    return;

  /* Draw the plot line */
  cairo_new_path (cr);
  p = poc_point_array_index (priv->points, 0);
  cairo_move_to (cr, poc_axis_project (priv->x_axis, p.x, width),
		     poc_axis_project (priv->y_axis, p.y, -height));
  for (i = 1; i < poc_point_array_len (priv->points); i++)
    {
      p = poc_point_array_index (priv->points, i);
      cairo_line_to (cr, poc_axis_project (priv->x_axis, p.x, width),
			 poc_axis_project (priv->y_axis, p.y, -height));
    }

  /* Stroke the line */
  cairo_set_line_width (cr, 1.0);
  dashes = poc_line_style_get_dashes (priv->line_style, &num_dashes);
  cairo_set_dash (cr, dashes, num_dashes, 0.0);
  gdk_cairo_set_source_rgba (cr, &priv->line_stroke);
  cairo_stroke (cr);
}

