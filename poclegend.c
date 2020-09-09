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
 * PocLegend
 */

#include "poclegend.h"
#include <math.h>

/**
 * SECTION: poclegend
 * @title:  PocLegend
 * @short_description: Widget to display a legend for a #PocPlot.
 * @see_also: #PocPlot
 *
 * #PocLegend displays a legend for the specified #PocPlot widget.
 * Plot and dataset titles are used for legend text and dataset
 * line styles and colours are used for the line samples.
 *
 * Please note that although operational, PocLegend is more of a
 * proof-of-concept at present and needs some work on its aesthetics.
 */

typedef struct _PocLegend PocLegend;
struct _PocLegend
  {
    GtkDrawingArea    parent_instance;

    PocPlot *plot;
    gdouble title_text_size;
    gdouble legend_text_size;
    gdouble line_sample_size;
    gdouble line_spacing;

    gdouble width;
    gboolean relayout;
  };

G_DEFINE_TYPE (PocLegend, poc_legend, GTK_TYPE_DRAWING_AREA)

#if !GLIB_CHECK_VERSION(2,62,0)
static inline void
g_clear_signal_handler (gulong *id, gpointer instance)
{
  if (instance != NULL && *id != 0)
    g_signal_handler_disconnect (instance, *id);
  *id = 0;
}
#endif

/**
 * poc_legend_new:
 *
 * Create a new #PocLegend
 *
 * Returns: (transfer full): New #PocLegend
 */
PocLegend *
poc_legend_new (void)
{
  return g_object_new (POC_TYPE_LEGEND, NULL);
}

enum
  {
    PROP_0,
    PROP_PLOT,
    PROP_TITLE_TEXT_SIZE,
    PROP_LEGEND_TEXT_SIZE,
    PROP_LINE_SAMPLE_SIZE,
    PROP_LINE_SPACING,
    N_PROPERTIES
  };
static GParamSpec *poc_legend_prop[N_PROPERTIES];

static void poc_legend_dispose (GObject *object);
static void poc_legend_finalize (GObject *object);
static void poc_legend_get_property (GObject *object, guint param_id,
				     GValue *value, GParamSpec *pspec);
static void poc_legend_set_property (GObject *object, guint param_id,
				     const GValue *value, GParamSpec *pspec);
static gboolean poc_legend_draw (GtkWidget *widget, cairo_t *cr);

/* GObject {{{1 */

static void
poc_legend_class_init (PocLegendClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);

  gobject_class->dispose = poc_legend_dispose;
  gobject_class->finalize = poc_legend_finalize;
  gobject_class->set_property = poc_legend_set_property;
  gobject_class->get_property = poc_legend_get_property;

  gtk_widget_class_set_css_name (widget_class, "legend");
  widget_class->draw = poc_legend_draw;

  poc_legend_prop[PROP_PLOT] = g_param_spec_object (
        "plot", "Plot",
        "Generate legend for the specified plot",
        POC_TYPE_PLOT,
        G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);
  poc_legend_prop[PROP_TITLE_TEXT_SIZE] = g_param_spec_double (
        "title-text-size", "Title Text Size",
        "Size for title text",
        1.0, G_MAXDOUBLE, 12.0,
        G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);
  poc_legend_prop[PROP_LEGEND_TEXT_SIZE] = g_param_spec_double (
        "legend-text-size", "Legend Text Size",
        "Size for legend text",
        1.0, G_MAXDOUBLE, 10.0,
        G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);
  poc_legend_prop[PROP_LINE_SAMPLE_SIZE] = g_param_spec_double (
        "line-sample-size", "Line Sample Size",
        "Length of sample line",
        1.0, G_MAXDOUBLE, 50.0,
        G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);
  poc_legend_prop[PROP_LINE_SPACING] = g_param_spec_double (
        "line-spacing", "Line Spacing",
        "Factor to scale line spacing",
        0.8, 5.0, 1.0,
        G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (gobject_class, N_PROPERTIES, poc_legend_prop);
}

static void
poc_legend_init (PocLegend *self)
{
  self->plot = NULL;
  self->title_text_size = 12.0;
  self->legend_text_size = 10.0;
  self->line_sample_size = 50.0;
  self->line_spacing = 1.0;
}

static void
poc_legend_dispose (GObject *object)
{
  PocLegend *self = (PocLegend *) object;

  g_clear_object (&self->plot);
  G_OBJECT_CLASS (poc_legend_parent_class)->dispose (object);
}

static void
poc_legend_finalize (GObject *object)
{
  //PocLegend *self = (PocLegend *) object;

  G_OBJECT_CLASS (poc_legend_parent_class)->finalize (object);
}

/* Properties {{{1 */

static void
poc_legend_set_property (GObject *object, guint prop_id,
			 const GValue *value, GParamSpec *pspec)
{
  PocLegend *self = POC_LEGEND (object);

  switch (prop_id)
    {
    case PROP_PLOT:
      poc_legend_set_plot (self, g_value_get_object (value));
      break;
    case PROP_TITLE_TEXT_SIZE:
      poc_legend_set_title_text_size (self, g_value_get_double (value));
      break;
    case PROP_LEGEND_TEXT_SIZE:
      poc_legend_set_legend_text_size (self, g_value_get_double (value));
      break;
    case PROP_LINE_SAMPLE_SIZE:
      poc_legend_set_line_sample_size (self, g_value_get_double (value));
      break;
    case PROP_LINE_SPACING:
      poc_legend_set_line_spacing (self, g_value_get_double (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      return;
    }
}


static void
poc_legend_get_property (GObject *object, guint prop_id,
			 GValue *value, GParamSpec *pspec)
{
  PocLegend *self = POC_LEGEND (object);

  switch (prop_id)
    {
    case PROP_PLOT:
      g_value_set_object (value, poc_legend_get_plot (self));
      break;
    case PROP_TITLE_TEXT_SIZE:
      g_value_set_double (value, poc_legend_get_title_text_size (self));
      break;
    case PROP_LEGEND_TEXT_SIZE:
      g_value_set_double (value, poc_legend_get_legend_text_size (self));
      break;
    case PROP_LINE_SAMPLE_SIZE:
      g_value_set_double (value, poc_legend_get_line_sample_size (self));
      break;
    case PROP_LINE_SPACING:
      g_value_set_double (value, poc_legend_get_line_spacing (self));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

/* "plot" {{{2 */

/**
 * poc_legend_set_plot:
 * @self: A #PocLegend
 * @plot: A #PocPlot
 *
 * Display a legend for the associated plot widget.
 */
void
poc_legend_set_plot (PocLegend *self, PocPlot *plot)
{
  g_return_if_fail (POC_IS_LEGEND (self));

  if (g_set_object (&self->plot, plot))
    {
      self->relayout = TRUE;
      gtk_widget_queue_draw (GTK_WIDGET (self));
      g_object_notify_by_pspec (G_OBJECT (self), poc_legend_prop[PROP_PLOT]);
    }
}

/**
 * poc_legend_get_plot:
 * @self: A #PocLegend
 *
 * Return the associated plot.
 *
 * Returns: (transfer none): a #PocPlot
 */
PocPlot *
poc_legend_get_plot (PocLegend *self)
{
  g_return_val_if_fail (POC_IS_LEGEND (self), NULL);

  return self->plot;
}

/* "title-text-size" {{{2 */

/**
 * poc_legend_set_title_text_size:
 * @self: A #PocLegend
 * @title_text_size: text size
 *
 * Set title text size.
 */
void
poc_legend_set_title_text_size (PocLegend *self, gdouble title_text_size)
{
  g_return_if_fail (POC_IS_LEGEND (self));

  if (self->title_text_size != title_text_size)
    {
      self->title_text_size = title_text_size;
      gtk_widget_queue_draw (GTK_WIDGET (self));
      g_object_notify_by_pspec (G_OBJECT (self), poc_legend_prop[PROP_TITLE_TEXT_SIZE]);
    }
}

/**
 * poc_legend_get_title_text_size:
 * @self: A #PocLegend
 *
 * Get title text size.
 *
 * Returns: size
 */
gdouble
poc_legend_get_title_text_size (PocLegend *self)
{
  g_return_val_if_fail (POC_IS_LEGEND (self), 0);

  return self->title_text_size;
}

/* "legend-text-size" {{{2 */

/**
 * poc_legend_set_legend_text_size:
 * @self: A #PocLegend
 * @legend_text_size: text size
 *
 * Set legend text size.
 */
void
poc_legend_set_legend_text_size (PocLegend *self, gdouble legend_text_size)
{
  g_return_if_fail (POC_IS_LEGEND (self));

  if (self->legend_text_size != legend_text_size)
    {
      self->legend_text_size = legend_text_size;
      gtk_widget_queue_draw (GTK_WIDGET (self));
      g_object_notify_by_pspec (G_OBJECT (self), poc_legend_prop[PROP_LEGEND_TEXT_SIZE]);
    }
}

/**
 * poc_legend_get_legend_text_size:
 * @self: A #PocLegend
 *
 * Get legend text size.
 *
 * Returns: size
 */
gdouble
poc_legend_get_legend_text_size (PocLegend *self)
{
  g_return_val_if_fail (POC_IS_LEGEND (self), 0);

  return self->legend_text_size;
}

/* "line-sample-size" {{{2 */

/**
 * poc_legend_set_line_sample_size:
 * @self: A #PocLegend
 * @line_sample_size: text size
 *
 * Set line sample size.
 */
void
poc_legend_set_line_sample_size (PocLegend *self, gdouble line_sample_size)
{
  g_return_if_fail (POC_IS_LEGEND (self));

  if (self->line_sample_size != line_sample_size)
    {
      self->line_sample_size = line_sample_size;
      gtk_widget_queue_draw (GTK_WIDGET (self));
      g_object_notify_by_pspec (G_OBJECT (self), poc_legend_prop[PROP_LINE_SAMPLE_SIZE]);
    }
}

/**
 * poc_legend_get_line_sample_size:
 * @self: A #PocLegend
 *
 * Get line sample size.
 *
 * Returns: size
 */
gdouble
poc_legend_get_line_sample_size (PocLegend *self)
{
  g_return_val_if_fail (POC_IS_LEGEND (self), 0);

  return self->line_sample_size;
}

/* "line-spacing" {{{2 */

/**
 * poc_legend_set_line_spacing:
 * @self: A #PocLegend
 * @line_spacing: text size
 *
 * Set line spacing as a multiple of the text height where 1.0 equals text
 * height.
 */
void
poc_legend_set_line_spacing (PocLegend *self, gdouble line_spacing)
{
  g_return_if_fail (POC_IS_LEGEND (self));

  if (self->line_spacing != line_spacing)
    {
      self->line_spacing = line_spacing;
      g_object_notify_by_pspec (G_OBJECT (self), poc_legend_prop[PROP_LINE_SPACING]);
    }
}

/**
 * poc_legend_get_line_spacing:
 * @self: A #PocLegend
 *
 * Get line spacing.
 *
 * Returns: size
 */
gdouble
poc_legend_get_line_spacing (PocLegend *self)
{
  g_return_val_if_fail (POC_IS_LEGEND (self), 0);

  return self->line_spacing;
}

/* layout {{{1 */

struct closure
  {
    PocLegend *self;
    cairo_t *cr;
    cairo_font_extents_t font_extents;
    GdkRGBA foreground;
    gint count;
    gdouble y;
    gdouble width;
    gdouble height;
  };

#define INTERNAL_BORDER	6.0

#if X
static gboolean
poc_legend_dimensions (G_GNUC_UNUSED PocPlot *plot, PocDataset *dataset,
		       gpointer user_data)
{
  struct closure *closure = user_data;
  PocLegend *self = closure->self;
  cairo_t *cr = closure->cr;
  cairo_text_extents_t text_extents;
  const gchar *legend;
  gdouble width;

  width = self->line_sample_size;
  legend = poc_dataset_get_legend (dataset);
  if (legend != NULL)
    {
      cairo_text_extents (cr, legend, &text_extents);
      width += text_extents.width + INTERNAL_BORDER;
    }
  if (width > closure->width)
    closure->width = width;
  closure->count += 1;
  return FALSE;
}

static void
poc_legend_layout (PocLegend *self, cairo_t *cr)
{
  struct closure closure;
  cairo_text_extents_t text_extents;
  const gchar *title;
  double line_height;

  closure.self = self;
  closure.cr = cr;
  closure.count = 0;

  title = poc_plot_get_title (self->plot);
  if (title != NULL)
    {
      cairo_set_font_size (cr, self->title_text_size);
      cairo_font_extents (cr, &closure.font_extents);
      cairo_text_extents (cr, title, &text_extents);
      closure.width = text_extents.width;
      closure.height = closure.font_extents.height;
    }
  else
    closure.height = closure.width = 0.0;

  cairo_set_font_size (cr, self->legend_text_size);
  cairo_font_extents (cr, &closure.font_extents);
  poc_plot_dataset_foreach (self->plot, poc_legend_dimensions, &closure);

  line_height = closure.font_extents.height * self->line_spacing;
  closure.height += line_height * closure.count;

  gtk_widget_set_size_request (GTK_WIDGET (self),
			       (gint) closure.width, (gint) closure.height);
}
#endif

/* draw {{{1 */

static gboolean
poc_legend_sample (G_GNUC_UNUSED PocPlot *plot, PocDataset *dataset,
		   gpointer user_data)
{
  struct closure *closure = user_data;
  PocLegend *self = closure->self;
  cairo_t *cr = closure->cr;
  cairo_text_extents_t text_extents;
  GdkRGBA rgba;
  const double *dashes;
  int num_dashes;
  PocLineStyle line_style;
  const gchar *legend;
  double line_height, line_ascent, x, y;

  line_height = closure->font_extents.height * self->line_spacing;
  line_ascent = closure->font_extents.ascent * self->line_spacing;

  legend = poc_dataset_get_legend (dataset);
  if (legend != NULL)
    {
      gdk_cairo_set_source_rgba (cr, &closure->foreground);
      cairo_text_extents (cr, legend, &text_extents);
      x = round ((closure->width * 1.5 - text_extents.width) / 2.0);
      y = round (closure->y + line_ascent);
      cairo_move_to (cr, x, y);
      cairo_show_text (cr, legend);
    }

  /* fetch sample parameters from dataset */
  poc_dataset_get_line_stroke (dataset, &rgba);
  line_style = poc_dataset_get_line_style (dataset);
  dashes = poc_line_style_get_dashes (line_style, &num_dashes);
  cairo_set_dash (cr, dashes, num_dashes, 0.0);
  gdk_cairo_set_source_rgba (cr, &rgba);

  /* stroke the sample line */
  x = round ((closure->width / 2.0 - self->line_sample_size) / 2.0);
  y = round (closure->y + line_height / 2.0);
  cairo_move_to (cr, x + 0.5, y + 0.5);
  cairo_rel_line_to (cr, self->line_sample_size, 0.0);
  cairo_stroke (cr);

  closure->y += line_height;
  return FALSE;
}

static gboolean
poc_legend_draw (GtkWidget *widget, cairo_t *cr)
{
  PocLegend *self = (PocLegend *) widget;
  cairo_text_extents_t text_extents;
  struct closure closure;
  const gchar *title;
  GtkStyleContext *style;
  GtkStateFlags state;

  if (self->plot == NULL)
    return FALSE;

#if X
  if (self->relayout)
    {
      poc_legend_layout (self, cr);
      self->relayout = FALSE;
    }
#endif

  style = gtk_widget_get_style_context (widget);
  state = gtk_style_context_get_state (style);
  gtk_style_context_get_color (style, state, &closure.foreground);

  closure.self = self;
  closure.cr = cr;
  closure.width = gtk_widget_get_allocated_width (widget);
  //closure.height = gtk_widget_get_allocated_height (widget);

  title = poc_plot_get_title (self->plot);
  if (title != NULL)
    {
      cairo_set_font_size (cr, self->title_text_size);
      cairo_font_extents (cr, &closure.font_extents);
      cairo_text_extents (cr, title, &text_extents);

      cairo_move_to (cr, round ((closure.width - text_extents.width) / 2.0),
			 round (closure.font_extents.ascent));
      gdk_cairo_set_source_rgba (cr, &closure.foreground);
      cairo_show_text (cr, title);

      closure.y = closure.font_extents.height;
    }
  else
    closure.y = 0;

  cairo_set_font_size (cr, self->legend_text_size);
  cairo_font_extents (cr, &closure.font_extents);
  cairo_set_line_width (cr, 1.0);
  poc_plot_dataset_foreach (self->plot, poc_legend_sample, &closure);

  return FALSE;
}
