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
#include "pocplot.h"
#include "pocdataset.h"
#include "pocaxis.h"
#include "pocbag.h"
#include "poctypes.h"

#include <stdlib.h>

/**
 * SECTION: pocplot
 * @title:  PocPlot
 * @short_description: #PocPlot canvas widget
 * @see_also: #PocAxis #PocDataset #PocLegend #PocSample
 *
 * #PocPlot is a 2D graph plotting canvas. To create a plot, add #PocAxis and
 * #PocDataset gadgets to this widget, see poc_plot_add_dataset() and
 * poc_plot_add_axis().
 *
 * # PocPlot as GtkBuildable
 *
 * The PocPlot implementation supports a custom `<plot>` element which supports
 * any number of `<dataset>` and `<axis>` elements.
 *
 * An example of a UI definition fragment for a PocPlot.
 * |[
 *  <object class="PocPlot">
 *    <plot>
 *      <dataset source="blue"/>
 *      <axis source="red" orientation="vertical" hidden="false"/>
 *    </plot>
 *  </object>
 *  <object class="PocDataset" id="blue">
 *    ...
 *  </object>
 *  <object class="PocAxis" id="red">
 *    ...
 *  </object>
 * ]|
 *
 * The `<dataset>` element supports the following attributes:
 *
 * - source: Xml id of a PocDataset element.
 * - x-pack: an optional #GtkPackType, how to pack the dataset's X axis.
 * - y-pack: an optional #GtkPackType, how to pack the dataset's Y axis.
 *
 * The `<axis>` elements support the following attribute:
 *
 * - source: Xml id of a PocAxis element.
 * - pack: an optional #GtkPackType, how to pack the axis.
 * - orientation: a #GtkOrientation, set the Axis orientation
 * - hidden: an optional #gboolean, whether the axis should be displayed or hidden.
 */

struct _PocPlot
{
  GtkDrawingArea	parent_instance;

  GdkRGBA		plot_fill;
  gfloat		border;

  gchar			*title;

  PocObjectBag		*axes;
  PocObjectBag		*datasets;
  PocAxis		*x_axis;
  PocAxis		*y_axis;

  GdkRectangle		area;
  gint			width, height;

  gint			solo;
  guint			enable_plot_fill : 1;
  guint			relayout : 1;
};

typedef struct _PocPlotAxis PocPlotAxis;
struct _PocPlotAxis
  {
    GtkPackType pack;
    GtkOrientation orientation;
    gboolean hidden;
    GdkRectangle area;
  };

typedef struct _PocPlotDataset PocPlotDataset;
struct _PocPlotDataset
  {
    gboolean solo;
  };

static void poc_plot_buildable_init (GtkBuildableIface *iface);

G_DEFINE_TYPE_WITH_CODE (PocPlot, poc_plot, GTK_TYPE_DRAWING_AREA,
	 G_IMPLEMENT_INTERFACE (GTK_TYPE_BUILDABLE, poc_plot_buildable_init))

/**
 * poc_plot_new:
 *
 * Create a new #PocPlot
 *
 * Returns: (transfer full): New #PocPlot
 */
PocPlot *
poc_plot_new (void)
{
  return g_object_new (POC_TYPE_PLOT, NULL);
}

/* GObject {{{1 */

enum
  {
    PROP_0,

    PROP_ENABLE_PLOT_FILL,
    PROP_PLOT_FILL,
    PROP_BORDER,

    PROP_TITLE,

    PROP_X_AXIS,
    PROP_Y_AXIS,

    N_PROPERTIES
  };
static GParamSpec *poc_plot_prop[N_PROPERTIES];

static void poc_plot_dispose (GObject *object);
static void poc_plot_finalize (GObject *object);
static void poc_plot_get_property (GObject *object, guint param_id,
				   GValue *value, GParamSpec *pspec);
static void poc_plot_set_property (GObject *object, guint param_id,
				   const GValue *value, GParamSpec *pspec);
static gboolean poc_plot_draw (GtkWidget *widget, cairo_t *cr);

static void
poc_plot_class_init (PocPlotClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);

  gobject_class->dispose = poc_plot_dispose;
  gobject_class->finalize = poc_plot_finalize;
  gobject_class->set_property = poc_plot_set_property;
  gobject_class->get_property = poc_plot_get_property;

  gtk_widget_class_set_css_name (widget_class, "plot");
  widget_class->draw = poc_plot_draw;

  /*FIXME - the following should be specified by CSS */
  poc_plot_prop[PROP_ENABLE_PLOT_FILL] = g_param_spec_boolean (
  	"enable-plot-fill",
	"Enable plot Fill", "Enable plot fill colour",
	FALSE,
	G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY |
	G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  poc_plot_prop[PROP_PLOT_FILL] = g_param_spec_boxed (
	"plot-fill",
	"Plot Fill Colour", "Plot fill colour for graph area.",
	GDK_TYPE_RGBA,
	G_PARAM_EXPLICIT_NOTIFY |
	G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  poc_plot_prop[PROP_BORDER] = g_param_spec_float (
	"border",
	"Internal Border", "Internal border width between plot items",
	0.0f, 100.0f, 6.0f,
	G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY |
	G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  poc_plot_prop[PROP_TITLE] = g_param_spec_string (
	"title",
	"Title", "Plot title.",
	NULL,
	G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY |
	G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  poc_plot_prop[PROP_X_AXIS] = g_param_spec_object (
        "x-axis", "X Axis",
        "Current X axis - draw X coordinate grid for this axis",
        POC_TYPE_AXIS,
        G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);
  poc_plot_prop[PROP_Y_AXIS] = g_param_spec_object (
        "y-axis", "Y Axis",
        "Current X axis - draw Y coordinate grid for this axis",
        POC_TYPE_AXIS,
        G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (gobject_class, N_PROPERTIES, poc_plot_prop);
}

static void
poc_plot_init (PocPlot *self)
{
  GdkRGBA white = { 1.0, 1.0, 1.0, 1.0 };

  self->plot_fill = white;

  self->axes = poc_object_bag_new ();
  self->datasets = poc_object_bag_new ();
}

static void
poc_plot_dispose (GObject *object)
{
  PocPlot *self = (PocPlot *) object;

  g_clear_object (&self->x_axis);
  g_clear_object (&self->y_axis);
  if (self->datasets != NULL)
    poc_object_bag_unref (self->datasets);
  self->datasets = NULL;
  if (self->axes != NULL)
    poc_object_bag_unref (self->axes);
  self->axes = NULL;
  G_OBJECT_CLASS (poc_plot_parent_class)->dispose (object);
}

static void
poc_plot_finalize (GObject *object)
{
  PocPlot *self = (PocPlot *) object;

  g_free (self->title);
  G_OBJECT_CLASS (poc_plot_parent_class)->finalize (object);
}

static void
poc_plot_set_property (GObject *object, guint prop_id,
		       const GValue *value, GParamSpec *pspec)
{
  PocPlot *self = POC_PLOT (object);

  switch (prop_id)
    {
    case PROP_ENABLE_PLOT_FILL:
      poc_plot_set_enable_plot_fill (self, g_value_get_boolean (value));
      break;
    case PROP_PLOT_FILL:
      poc_plot_set_plot_fill (self, g_value_get_boxed (value));
      break;
    case PROP_BORDER:
      poc_plot_set_border (self, g_value_get_float (value));
      break;

    case PROP_TITLE:
      poc_plot_set_title (self, g_value_get_string (value));
      break;

    case PROP_X_AXIS:
      poc_plot_set_x_axis (self, g_value_get_object (value));
      break;
    case PROP_Y_AXIS:
      poc_plot_set_y_axis (self, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      return;
    }
}

static void
poc_plot_get_property (GObject *object, guint prop_id,
		       GValue *value, GParamSpec *pspec)
{
  PocPlot *self = POC_PLOT (object);
  GdkRGBA rgba;

  switch (prop_id)
    {
    case PROP_ENABLE_PLOT_FILL:
      g_value_set_boolean (value, poc_plot_get_enable_plot_fill (self));
      break;
    case PROP_PLOT_FILL:
      poc_plot_get_plot_fill (self, &rgba);
      g_value_set_boxed (value, &rgba);
      break;
    case PROP_BORDER:
      g_value_set_float (value, poc_plot_get_border (self));
      break;

    case PROP_TITLE:
      g_value_set_string (value, poc_plot_get_title (self));
      break;

    case PROP_X_AXIS:
      g_value_set_object (value, poc_plot_get_x_axis (self));
      break;
    case PROP_Y_AXIS:
      g_value_set_object (value, poc_plot_get_y_axis (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

/* Properties {{{1 */

/* enable plot fill  {{{2 */

/**
 * poc_plot_set_enable_plot_fill:
 * @self: A #PocPlot
 * @value: %TRUE to enable plot fill.
 *
 * Set whether to fill the plot area plot.
 */
void
poc_plot_set_enable_plot_fill (PocPlot *self, gboolean value)
{
  self->enable_plot_fill = !!value;
  poc_plot_notify_update (self);
  g_object_notify_by_pspec (G_OBJECT (self), poc_plot_prop[PROP_ENABLE_PLOT_FILL]);
}

/**
 * poc_plot_get_enable_plot_fill:
 * @self: A #PocPlot
 *
 * Get whether to fill the plot area plot.
 *
 * Returns: %TRUE if plot fill is enabled.
 */
gboolean
poc_plot_get_enable_plot_fill (PocPlot *self)
{
  return self->enable_plot_fill;
}

/* plot fill {{{2 */

/**
 * poc_plot_get_plot_fill:
 * @self: A #PocPlot
 * @rgba: A #GdkRGBA with the plot background colour.
 *
 * Set the plot area background colour.
 */
void
poc_plot_get_plot_fill (PocPlot *self, GdkRGBA *rgba)
{
  g_return_if_fail (POC_IS_PLOT (self));
  g_return_if_fail (rgba != NULL);

  *rgba = self->plot_fill;
}

/**
 * poc_plot_set_plot_fill:
 * @self: A #PocPlot
 * @rgba: A #GdkRGBA to receive the plot background colour.
 *
 * Get the plot area plot colour.
 */
void
poc_plot_set_plot_fill (PocPlot *self, const GdkRGBA *rgba)
{
  g_return_if_fail (POC_IS_PLOT (self));
  g_return_if_fail (rgba != NULL);

  self->plot_fill = *rgba;
  poc_plot_notify_update (self);
  g_object_notify_by_pspec (G_OBJECT (self), poc_plot_prop[PROP_PLOT_FILL]);
}

/* border {{{1 */

/**
 * poc_plot_set_border:
 * @self: A #PocPlot
 * @size: border width
 *
 * Set the internal border width.
 */
void
poc_plot_set_border (PocPlot *self, gfloat size)
{
  g_return_if_fail (POC_IS_PLOT (self));

  self->border = size;
  g_object_notify_by_pspec (G_OBJECT (self), poc_plot_prop[PROP_BORDER]);
}

/**
 * poc_plot_get_border:
 * @self: A #PocPlot
 *
 * Get the internal border width.
 *
 * Returns: border width
 */
gfloat
poc_plot_get_border (PocPlot *self)
{
  g_return_val_if_fail (POC_IS_PLOT (self), 0.0);

  return self->border;
}

/* title {{{2 */

/**
 * poc_plot_set_title:
 * @self: A #PocPlot
 * @title: Title for the plot.
 *
 * Set the plot title. The title is not used directly by the #PocPlot widget,
 * however it may be used by other gadgets added to the plot.
 */
void
poc_plot_set_title (PocPlot *self, const gchar *title)
{
  g_return_if_fail (POC_IS_PLOT (self));

  g_free (self->title);
  self->title = g_strdup (title);
  poc_plot_notify_update (self);
  g_object_notify_by_pspec (G_OBJECT (self), poc_plot_prop[PROP_TITLE]);
}

/**
 * poc_plot_get_title:
 * @self: A #PocPlot
 *
 * Set the plot title.
 *
 * Returns: (transfer none): Plot title.
 */
const gchar *
poc_plot_get_title (PocPlot *self)
{
  g_return_val_if_fail (POC_IS_PLOT (self), NULL);

  return self->title;
}

/* "x-axis" {{{2 */

/**
 * poc_plot_set_x_axis:
 * @self: A #PocPlot
 * @x_axis: #PocAxis to set the current plot grid
 *
 * Set the current X axis.  This is used to draw the X axis grid lines in the
 * plot area.
 */
void
poc_plot_set_x_axis (PocPlot *self, PocAxis *x_axis)
{
  g_return_if_fail (POC_IS_PLOT (self));
  g_return_if_fail (POC_IS_AXIS (x_axis));

  if (poc_object_bag_contains (self->axes, G_OBJECT (x_axis))
      && g_set_object (&self->x_axis, x_axis))
    {
      poc_plot_notify_update (self);
      g_object_notify_by_pspec (G_OBJECT (self), poc_plot_prop[PROP_X_AXIS]);
    }
}

/**
 * poc_plot_get_x_axis:
 * @self: A #PocPlot
 *
 * Get the current X axis.
 *
 * Returns: (transfer none): current X-Axis
 */
PocAxis *
poc_plot_get_x_axis (PocPlot *self)
{
  g_return_val_if_fail (POC_IS_PLOT (self), NULL);

  return self->x_axis;
}

/* "y-axis" {{{2 */

/**
 * poc_plot_set_y_axis:
 * @self: A #PocPlot
 * @y_axis: #PocAxis to set the current plot grid
 *
 * Set the current Y axis.  This is used to draw the Y axis grid lines in the
 * plot area.
 */
void
poc_plot_set_y_axis (PocPlot *self, PocAxis *y_axis)
{
  g_return_if_fail (POC_IS_PLOT (self));
  g_return_if_fail (POC_IS_AXIS (y_axis));

  if (poc_object_bag_contains (self->axes, G_OBJECT (y_axis))
      && g_set_object (&self->y_axis, y_axis))
    {
      poc_plot_notify_update (self);
      g_object_notify_by_pspec (G_OBJECT (self), poc_plot_prop[PROP_Y_AXIS]);
    }
}

/**
 * poc_plot_get_y_axis:
 * @self: A #PocPlot
 *
 * Get the current Y axis.
 *
 * Returns: (transfer none): current Y-Axis
 */
PocAxis *
poc_plot_get_y_axis (PocPlot *self)
{
  g_return_val_if_fail (POC_IS_PLOT (self), NULL);

  return self->y_axis;
}

/**
 * poc_plot_set_axis:
 * @self: A #PocPlot
 * @axis: #PocAxis to set the current plot grid
 *
 * Set the current axis. The #PocPlot determines whether @axis refers to an
 * X or Y axis.
 */
void
poc_plot_set_axis (PocPlot *self, PocAxis *axis)
{
  PocPlotAxis *axis_data;

  g_return_if_fail (POC_IS_PLOT (self));

  axis_data = poc_object_bag_get_data (self->axes, G_OBJECT (axis));
  g_return_if_fail (axis_data != NULL);

  if (axis_data->orientation == GTK_ORIENTATION_HORIZONTAL)
    poc_plot_set_x_axis (self, axis);
  else
    poc_plot_set_y_axis (self, axis);
}

/* draw {{{1 */

struct poc_plot_closure
  {
    PocPlot *self;
    cairo_t *cr;
    GtkStyleContext *style;
    GdkRectangle area;
    guint x_offset_start;
    guint x_offset_end;
    guint y_offset_start;
    guint y_offset_end;
  };

static void poc_plot_layout (PocPlot *self, struct poc_plot_closure *closure);

static void
poc_plot_draw_dataset (gpointer object, gpointer object_data,
		       gpointer user_data)
{
  PocDataset *dataset = POC_DATASET (object);
  PocPlotDataset *dataset_data = object_data;
  struct poc_plot_closure *closure = user_data;
  PocPlot *self = closure->self;

  if (self->solo == 0 || dataset_data->solo)
    poc_dataset_draw (dataset, closure->cr, self->area.width, self->area.height);
}

static void
poc_plot_draw_axis (gpointer object, gpointer object_data, gpointer user_data)
{
  PocAxis *axis = object;
  PocPlotAxis *axis_data = object_data;
  struct poc_plot_closure *closure = user_data;

  if (axis_data->hidden)
    return;

  cairo_save (closure->cr);
  gdk_cairo_rectangle (closure->cr, &axis_data->area);
  cairo_clip (closure->cr);
  cairo_translate (closure->cr, axis_data->area.x, axis_data->area.y);
  poc_axis_draw_axis (axis, closure->cr,
		      axis_data->orientation, axis_data->pack,
		      axis_data->area.width, axis_data->area.height,
		      closure->style);
  cairo_restore (closure->cr);
}

static gboolean
poc_plot_draw (GtkWidget *widget, cairo_t *cr)
{
  PocPlot *self = (PocPlot *) widget;
  struct poc_plot_closure closure = { NULL, NULL, NULL, { 0, 0, 0, 0 }, 0, 0, 0, 0 };
  GtkStyleContext *style;
  GtkBorder border;
  GtkStateFlags state;

  style = gtk_widget_get_style_context (widget);
  state = gtk_style_context_get_state (style);

  closure.self = self;
  closure.cr = cr;
  closure.area.width = gtk_widget_get_allocated_width (widget);
  closure.area.height = gtk_widget_get_allocated_height (widget);
  closure.style = style;

  gtk_render_background (style, cr, closure.area.x, closure.area.y,
			 closure.area.width, closure.area.height);
  gtk_render_frame (style, cr, closure.area.x, closure.area.y,
  		    closure.area.width, closure.area.height);

  gtk_style_context_get_border (style, state, &border);
  closure.area.x += border.left;
  closure.area.y += border.top;
  closure.area.width -= border.right + border.left;
  closure.area.height -= border.bottom + border.top;

  gtk_style_context_get_padding (style, state, &border);
  closure.area.x += border.left;
  closure.area.y += border.top;
  closure.area.width -= border.right + border.left;
  closure.area.height -= border.bottom + border.top;

  if (self->relayout || closure.area.width != self->width || closure.area.height != self->height)
    {
      poc_plot_layout (self, &closure);
      self->relayout = FALSE;
      self->width = closure.area.width;
      self->height = closure.area.height;
    }

  /* Draw axes */
  poc_object_bag_foreach (self->axes, poc_plot_draw_axis, &closure);

  /**** draw the plot background ****/
  cairo_save (cr);
  gdk_cairo_rectangle (cr, &self->area);
  if (self->enable_plot_fill)
    {
      gdk_cairo_set_source_rgba (cr, &self->plot_fill);
      cairo_fill_preserve (cr);
    }
  cairo_clip (cr);
  cairo_translate (cr, self->area.x, self->area.y);

  /* Draw each dataset */
  poc_object_bag_foreach (self->datasets, poc_plot_draw_dataset, &closure);

  /* Draw the grid */
  if (self->x_axis != NULL)
    poc_axis_draw_grid (self->x_axis, cr, GTK_ORIENTATION_HORIZONTAL,
			self->area.width, self->area.height, style);
  if (self->y_axis != NULL)
    poc_axis_draw_grid (self->y_axis, cr, GTK_ORIENTATION_VERTICAL,
			self->area.width, self->area.height, style);

  cairo_restore (cr);

  return FALSE;
}

/* layout {{{1 */

static void
poc_plot_layout_axis1 (gpointer object, gpointer object_data, gpointer user_data)
{
  PocAxis *axis = object;
  PocPlotAxis *axis_data = object_data;
  struct poc_plot_closure *closure = user_data;
  guint size;

  if (axis_data->hidden)
    return;

  size = poc_axis_size (axis);
  if (axis_data->orientation == GTK_ORIENTATION_VERTICAL)
    {
      if (axis_data->pack == GTK_PACK_START)
	{
	  axis_data->area.x = closure->area.x + closure->x_offset_start;
	  axis_data->area.width = size;
	  closure->x_offset_start += size + closure->self->border;
	}
      else
	{
	  axis_data->area.x = closure->area.x + closure->area.width - 1
					      - (closure->x_offset_end + size);
	  axis_data->area.width = size;
	  closure->x_offset_end += size + closure->self->border;
	}
    }
  else
    {
      if (axis_data->pack == GTK_PACK_START)
	{
	  axis_data->area.y = closure->area.y + closure->area.height - 1
					    - (closure->y_offset_start + size);
	  axis_data->area.height = size;
	  closure->y_offset_start += size + closure->self->border;
	}
      else
	{
	  axis_data->area.y = closure->area.y + closure->y_offset_end;
	  axis_data->area.height = size;
	  closure->y_offset_end += size + closure->self->border;
	}
    }
}

static void
poc_plot_layout_axis2 (G_GNUC_UNUSED gpointer object, gpointer object_data,
		       gpointer user_data)
{
  PocPlotAxis *axis_data = object_data;
  struct poc_plot_closure *closure = user_data;

  if (axis_data->hidden)
    return;

  if (axis_data->orientation == GTK_ORIENTATION_VERTICAL)
    {
      axis_data->area.y = closure->self->area.y;
      axis_data->area.height = closure->self->area.height;
    }
  else
    {
      axis_data->area.x = closure->self->area.x;
      axis_data->area.width = closure->self->area.width;
    }
}

static void
poc_plot_layout (PocPlot *self, struct poc_plot_closure *closure)
{
  closure->x_offset_start = closure->y_offset_start = 0;
  closure->x_offset_end = closure->y_offset_end = 0;

  /* first pass set x coords for vertical axes, y for horizontal */
  poc_object_bag_foreach (self->axes, poc_plot_layout_axis1, closure);
  /* set the plot area */
  self->area.x = closure->area.x + closure->x_offset_start;
  self->area.width = closure->area.width - closure->x_offset_end
					 - closure->x_offset_start;
  self->area.y = closure->area.y + closure->y_offset_end;
  self->area.height = closure->area.height - closure->y_offset_start
  					   - closure->y_offset_end;
  //FIXME warn if insufficient area
  /* second pass set y coords for vertical axes, x for horizontal */
  poc_object_bag_foreach (self->axes, poc_plot_layout_axis2, closure);
}

/* methods {{{1 */

/**
 * poc_plot_add_dataset:
 * @self: A #PocPlot
 * @dataset: #PocDataset to add.
 * @x_pack: How to pack the X axis.
 * @y_pack: How to pack the Y axis (start = bottom, end = top).
 *
 * Add a #PocDataset gadget to the plot. @x_pack and @y_pack determine which
 * edge of the plot should show the axes.
 */
void
poc_plot_add_dataset (PocPlot *self, PocDataset *dataset,
		      GtkPackType x_pack, GtkPackType y_pack)
{
  PocAxis *axis;
  PocPlotDataset *data;

  g_return_if_fail (POC_IS_PLOT (self));
  g_return_if_fail (POC_IS_DATASET (dataset));

  g_object_freeze_notify (G_OBJECT (self));

  //XXX Dataset should probably be initiallyunowned and use ref_sink
  if (!poc_object_bag_add (self->datasets, G_OBJECT (dataset)))
    {
      data = g_new0 (PocPlotDataset, 1);
      poc_object_bag_set_data_full (self->datasets, G_OBJECT (dataset), data, g_free);

      g_signal_connect_object (dataset, "update",
			       G_CALLBACK (poc_plot_notify_update), self,
			       G_CONNECT_SWAPPED);
    }

  if ((axis = poc_dataset_get_x_axis (dataset)) != NULL)
    {
      poc_plot_add_axis (self, axis, FALSE, x_pack, GTK_ORIENTATION_HORIZONTAL);
      if (self->x_axis == NULL)
	poc_plot_set_x_axis (self, axis);
    }
  if ((axis = poc_dataset_get_y_axis (dataset)) != NULL)
    {
      poc_plot_add_axis (self, axis, FALSE, y_pack, GTK_ORIENTATION_VERTICAL);
      if (self->y_axis == NULL)
	poc_plot_set_y_axis (self, axis);
    }
  gtk_widget_queue_draw (GTK_WIDGET (self));
  g_object_thaw_notify (G_OBJECT (self));
}

static void
poc_plot_remove_dataset_internal (PocPlot *self, PocDataset *dataset)
{
  PocAxis *axis;

  g_signal_handlers_disconnect_by_func (dataset,
  					G_CALLBACK (poc_plot_notify_update),
					self);
  if ((axis = poc_dataset_get_x_axis (dataset)) != NULL)
    poc_plot_remove_axis (self, axis);
  if ((axis = poc_dataset_get_y_axis (dataset)) != NULL)
    poc_plot_remove_axis (self, axis);
}

/**
 * poc_plot_remove_dataset:
 * @self: A #PocPlot
 * @dataset: #PocDataset to remove
 *
 * Remove a @dataset from the plot. The @dataset must already belong to the
 * plot.  Axes that are no longer referenced by another #PocDataset are also
 * removed.
 */
void
poc_plot_remove_dataset (PocPlot *self, PocDataset *dataset)
{
  g_return_if_fail (POC_IS_PLOT (self));
  g_return_if_fail (POC_IS_DATASET (dataset));

  g_object_ref (dataset);
  if (poc_object_bag_remove (self->datasets, G_OBJECT (dataset)))
    {
      poc_plot_remove_dataset_internal (self, dataset);
      gtk_widget_queue_draw (GTK_WIDGET (self));
    }
  g_object_unref (dataset);
}

static void
poc_plot_clear_dataset_cb (gpointer object, G_GNUC_UNUSED gpointer object_data,
			   gpointer user_data)
{
  PocPlot *self = user_data;
  PocDataset *dataset = object;

  poc_plot_remove_dataset_internal (self, dataset);
}

/**
 * poc_plot_clear_dataset:
 * @self: A #PocPlot
 *
 * Remove all datasets from the plot.
 */
void
poc_plot_clear_dataset (PocPlot *self)
{
  poc_object_bag_foreach (self->datasets, poc_plot_clear_dataset_cb, self);
  poc_object_bag_empty (self->datasets);
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

static gboolean
poc_plot_dataset_compare (gpointer object, G_GNUC_UNUSED gpointer object_data,
			  gpointer user_data)
{
  PocDataset *dataset = object;
  const gchar *nickname = user_data;
  const gchar *ds_name;

  ds_name = poc_dataset_get_nickname (dataset);
  return g_strcmp0 (nickname, ds_name) == 0;
}

/**
 * poc_plot_find_dataset:
 * @self: A #PocPlot
 * @nickname: Nickname for a #PocDataset
 *
 * Find a dataset belonging to the plot with the requested nickname.
 *
 * returns: (transfer none): a #PocDataset or %NULL if not found.
 */
PocDataset *
poc_plot_find_dataset (PocPlot *self, const gchar *nickname)
{
  GObject *object;

  g_return_val_if_fail (POC_IS_PLOT (self), NULL);
  object = poc_object_bag_find (self->datasets, poc_plot_dataset_compare,
  				(gpointer) nickname);
  return object != NULL ? POC_DATASET (object) : NULL;
}

/**
 * poc_plot_solo_dataset:
 * @self: A #PocPlot.
 * @dataset: A #PocDataset belonging to the plot.
 * @solo: %TRUE to show only this @dataset.
 *
 * Show only the grid lines and plot data for the specified @dataset.  If
 * multiple datasets have solo enabled only they are displayed, if no datasets
 * are solo then all datasets are displayed.
 */
void
poc_plot_solo_dataset (PocPlot *self, PocDataset *dataset, gboolean solo)
{
  PocPlotDataset *data;

  g_return_if_fail (POC_IS_PLOT (self));
  g_return_if_fail (POC_IS_DATASET (dataset));

  data = poc_object_bag_get_data (self->datasets, G_OBJECT (dataset));
  g_return_if_fail (data != NULL);

  if (solo && !data->solo)
    self->solo += 1;
  else if (!solo && data->solo)
    self->solo -= 1;
  else
    return;
  data->solo = !!solo;
  g_assert (self->solo >= 0);
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

/**
 * poc_plot_add_axis:
 * @self: A #PocPlot
 * @axis: #PocAxis to add.
 * @hidden: Do not draw @axis if %TRUE
 * @pack: How to pack the axis.
 * @orientation: Orientation for the axis.
 *
 * Add an axis to the plot. @pack and @orientation specify how the axis should
 * be displayed. Normally this is not required as axes belonging to datasets
 * are added automatically.
 */
void
poc_plot_add_axis (PocPlot *self, PocAxis *axis, gboolean hidden,
		   GtkPackType pack, GtkOrientation orientation)
{
  PocPlotAxis *data;

  g_return_if_fail (POC_IS_PLOT (self));
  g_return_if_fail (POC_IS_AXIS (axis));

  //XXX Axis should probably be initiallyunowned and use ref_sink
  if (!poc_object_bag_add (self->axes, G_OBJECT (axis)))
    {
      data = g_new0 (PocPlotAxis, 1);
      data->hidden = hidden;
      data->pack = pack;
      data->orientation = orientation;
      poc_object_bag_set_data_full (self->axes, G_OBJECT (axis), data, g_free);

      g_signal_connect_object (axis, "update",
			       G_CALLBACK (poc_plot_notify_update), self,
			       G_CONNECT_SWAPPED);

      self->relayout = TRUE;
      gtk_widget_queue_draw (GTK_WIDGET (self));
    }
}

/**
 * poc_plot_remove_axis:
 * @self: A #PocPlot
 * @axis: #PocAxis to remove.
 *
 * Remove the @axis from the plot.
 */
void
poc_plot_remove_axis (PocPlot *self, PocAxis *axis)
{
  g_return_if_fail (POC_IS_PLOT (self));
  g_return_if_fail (POC_IS_AXIS (axis));

  if (poc_object_bag_remove (self->axes, G_OBJECT (axis)))
    {
      g_signal_handlers_disconnect_by_func (axis,
					    G_CALLBACK (poc_plot_notify_update),
					    self);
      if (axis == self->x_axis)
	{
	  g_clear_object (&self->x_axis);
	  g_object_notify_by_pspec (G_OBJECT (self), poc_plot_prop[PROP_X_AXIS]);
	}
      if (axis == self->y_axis)
	{
	  g_clear_object (&self->y_axis);
	  g_object_notify_by_pspec (G_OBJECT (self), poc_plot_prop[PROP_Y_AXIS]);
	}
      self->relayout = TRUE;
      gtk_widget_queue_draw (GTK_WIDGET (self));
    }
}

/**
 * poc_plot_clear_axes:
 * @self: A #PocPlot
 *
 * Remove all axes from the plot.
 */
void
poc_plot_clear_axes (PocPlot *self)
{
  g_clear_object (&self->x_axis);
  g_clear_object (&self->y_axis);
  poc_object_bag_empty (self->axes);

  self->relayout = TRUE;
  gtk_widget_queue_draw (GTK_WIDGET (self));
  g_object_notify_by_pspec (G_OBJECT (self), poc_plot_prop[PROP_X_AXIS]);
  g_object_notify_by_pspec (G_OBJECT (self), poc_plot_prop[PROP_Y_AXIS]);
}

struct point
  {
    gint x;
    gint y;
  };

static gboolean
poc_plot_is_axis_at_point (G_GNUC_UNUSED gpointer object, gpointer obj_data,
			   gpointer user_data)
{
  PocPlotAxis *axis_data = obj_data;
  struct point *point = user_data;

  return point->x >= axis_data->area.x
         && point->x < axis_data->area.x + axis_data->area.width
	 && point->y >= axis_data->area.y
         && point->y < axis_data->area.y + axis_data->area.height;
}

/**
 * poc_plot_axis_at_point:
 * @self: A #PocPlot
 * @x: x-coordinate in pixels
 * @y: y-coordinate in pixels
 *
 * Find the #PocAxis under the specifiec @x, @y coordinate.
 *
 * Returns: (transfer none): the #PocAxis.
 */
PocAxis *
poc_plot_axis_at_point (PocPlot *self, gdouble x, gdouble y)
{
  struct point point;
  GObject *obj;

  point.x = x;
  point.y = y;
  obj = poc_object_bag_find (self->axes, poc_plot_is_axis_at_point, &point);
  return POC_AXIS (obj);
}

/**
 * poc_plot_notify_update:
 * @self: A #PocPlot
 *
 * Notify PocPlot of updates in a dataset or axis.
 */
void
poc_plot_notify_update (PocPlot *self)
{
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

/* data and axis iterators {{{1 */

struct foreach_closure
  {
    PocPlot *self;
    GHRFunc predicate;
    gpointer user_data;
  };

static gboolean
foreach_wrapper (gpointer object, G_GNUC_UNUSED gpointer object_data,
		 gpointer user_data)
{
  struct foreach_closure *closure = user_data;

  return (*closure->predicate) (closure->self, object, closure->user_data);
}

/**
 * poc_plot_dataset_foreach:
 * @self: A #PocPlot
 * @predicate: (scope call): iterator predicate function.
 * @user_data: user data passed to @predicate.
 *
 * Call @predicate for each dataset in the plot.  If @predicate returns
 * %TRUE return the dataset. If iteration completes %NULL is returned.
 *
 * Returns: (transfer none): the #PocDataset or %NULL.
 */
PocDataset *
poc_plot_dataset_foreach (PocPlot *self, PocPlotDatasetForEachFunc predicate,
			  gpointer user_data)
{
  struct foreach_closure closure = { self, (GHRFunc) predicate, user_data };

  return (PocDataset *) poc_object_bag_find (self->datasets, foreach_wrapper, &closure);
}

/**
 * poc_plot_axis_foreach:
 * @self: A #PocPlot
 * @predicate: (scope call): iterator predicate function.
 * @user_data: user data passed to @predicate.
 *
 * Call @predicate for each axis in the plot.  If @predicate returns
 * %TRUE return the axis. If iteration completes %NULL is returned.
 *
 * Returns: (transfer none): the #PocAxis or %NULL.
 */
PocAxis *
poc_plot_axis_foreach (PocPlot *self, PocPlotAxisForEachFunc predicate,
		       gpointer user_data)
{
  struct foreach_closure closure = { self, (GHRFunc) predicate, user_data };

  return (PocAxis *) poc_object_bag_find (self->axes, foreach_wrapper, &closure);
}

/* buildable {{{1 */
/* implements
   <plot>
     <dataset source="id" [x-pack="start"] [y-pack="end"]/>
     <axis source="id" [orientation="horizontal"] [pack="start"]/>
   </plot>
 */

typedef struct _PocPlotParser PocPlotParser;
struct _PocPlotParser
  {
    GSList *dataset;
    GSList *axes;
  };

typedef struct _PocParserDataset PocParserDataset;
struct _PocParserDataset
  {
    gchar *source;
    GtkPackType x_pack;
    GtkPackType y_pack;
  };

static void
free_dataset_data (gpointer item)
{
  PocParserDataset *axis = item;

  g_free (axis->source);
  g_free (axis);
}

typedef struct _PocParserAxis PocParserAxis;
struct _PocParserAxis
  {
    gchar *source;
    gboolean hidden;
    GtkPackType pack;
    GtkOrientation orientation;
  };

static void
free_axis_data (gpointer item)
{
  PocParserAxis *axis = item;

  g_free (axis->source);
  g_free (axis);
}

/* sub parse {{{2 */

static gint
enum_from_string (GType enum_type, const gchar *string)
{
  GEnumClass *enum_class;
  GEnumValue *enum_value;

  enum_class = g_type_class_peek (enum_type);
  g_return_val_if_fail (enum_class != NULL, 0);
  enum_value = g_enum_get_value_by_name (enum_class, string);
  if (enum_value == NULL)
    enum_value = g_enum_get_value_by_nick (enum_class, string);
  if (enum_value == NULL)
    enum_value = g_enum_get_value (enum_class, strtol (string, NULL, 0));
  g_return_val_if_fail (enum_value != NULL, 0);

  return enum_value->value;
}

static void
poc_plot_parser_start_element (G_GNUC_UNUSED GMarkupParseContext *context,
			       const gchar *element_name,
			       const gchar **names, const gchar **values,
			       gpointer user_data, GError **error)
{
  PocPlotParser *data = (PocPlotParser *) user_data;
  gchar *source;
  const gchar *pack, *y_pack, *orientation;
  gboolean hidden;
  PocParserDataset *dataset;
  PocParserAxis *axis;

  if (strcmp (element_name, "dataset") == 0)
    {
      if (g_markup_collect_attributes (element_name, names, values, error,
				       G_MARKUP_COLLECT_STRDUP,
				       "source", &source,
				       G_MARKUP_COLLECT_STRING | G_MARKUP_COLLECT_OPTIONAL,
				       "x-pack", &pack,
				       G_MARKUP_COLLECT_STRING | G_MARKUP_COLLECT_OPTIONAL,
				       "y-pack", &y_pack,
				       G_MARKUP_COLLECT_INVALID))
	{
	  dataset = g_new0 (PocParserDataset, 1);
	  dataset->source = source;
	  dataset->x_pack = pack != NULL ? enum_from_string (GTK_TYPE_PACK_TYPE, pack) : GTK_PACK_START;
	  dataset->y_pack = y_pack != NULL ? enum_from_string (GTK_TYPE_PACK_TYPE, y_pack) : GTK_PACK_START;
	  data->dataset = g_slist_prepend (data->dataset, dataset);
	}
    }
  else if (strcmp (element_name, "axis") == 0)
    {
      if (g_markup_collect_attributes (element_name, names, values, error,
				       G_MARKUP_COLLECT_STRDUP,
				       "source", &source,
				       G_MARKUP_COLLECT_BOOLEAN | G_MARKUP_COLLECT_OPTIONAL,
				       "hidden", &hidden,
				       G_MARKUP_COLLECT_STRING | G_MARKUP_COLLECT_OPTIONAL,
				       "pack", &pack,
				       G_MARKUP_COLLECT_STRING,
				       "orientation", &orientation,
				       G_MARKUP_COLLECT_INVALID))
	{
	  axis = g_new0 (PocParserAxis, 1);
	  axis->source = source;
	  axis->hidden = hidden;
	  axis->pack = pack != NULL ? enum_from_string (GTK_TYPE_PACK_TYPE, pack) : GTK_PACK_START;
	  axis->orientation = enum_from_string (GTK_TYPE_ORIENTATION, orientation);
	  data->axes = g_slist_prepend (data->axes, axis);
	}
    }
}

static const GMarkupParser poc_plot_sub_parser =
  {
    poc_plot_parser_start_element,
    NULL,
    NULL,
    NULL,
    NULL,
  };

/* custom tag {{{2 */

static gboolean
poc_plot_buildable_custom_tag_start (G_GNUC_UNUSED GtkBuildable *buildable,
				     G_GNUC_UNUSED GtkBuilder *builder,
				     G_GNUC_UNUSED GObject *child,
				     const gchar *tagname,
				     GMarkupParser *parser,
				     gpointer *parser_data)
{
  PocPlotParser *data = NULL;

  if (strcmp (tagname, "plot") != 0)
    return FALSE;

  data = g_slice_new0 (PocPlotParser);
  *parser = poc_plot_sub_parser;
  *parser_data = data;
  return TRUE;
}

static void
poc_plot_buildable_custom_finished (GtkBuildable *buildable,
				    GtkBuilder *builder,
				    G_GNUC_UNUSED GObject *child,
				    const gchar *tagname,
				    gpointer user_data)
{
  PocPlot *self = POC_PLOT (buildable);
  PocPlotParser *data = user_data;
  GSList *item;
  GObject *object;
  PocParserDataset *dataset;
  PocParserAxis *axis;

  if (strcmp (tagname, "plot") != 0)
    return;

  data->axes = g_slist_reverse (data->axes);
  for (item = data->axes; item != NULL; item = item->next)
    {
      axis = item->data;
      object = gtk_builder_get_object (builder, axis->source);

      if (object != NULL && POC_IS_AXIS (object))
	poc_plot_add_axis (self, POC_AXIS (object),
			   axis->hidden, axis->pack, axis->orientation);
    }
  g_slist_free_full (data->axes, free_axis_data);

  data->dataset = g_slist_reverse (data->dataset);
  for (item = data->dataset; item != NULL; item = item->next)
    {
      dataset = item->data;
      object = gtk_builder_get_object (builder, dataset->source);

      if (object != NULL && POC_IS_DATASET (object))
	poc_plot_add_dataset (self, POC_DATASET (object),
			      dataset->x_pack, dataset->y_pack);
    }
  g_slist_free_full (data->dataset, free_dataset_data);

  g_slice_free (PocPlotParser, data);
}

/* buildable {{{2 */

static void
poc_plot_buildable_init (GtkBuildableIface *iface)
{
  iface->custom_tag_start = poc_plot_buildable_custom_tag_start;
  iface->custom_finished = poc_plot_buildable_custom_finished;
}
