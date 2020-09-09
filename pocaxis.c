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
#define _GNU_SOURCE	    /* See feature_test_macros(7) */
#include "pocaxis.h"
#include <math.h>
#include "mathextra.h"

/**
 * SECTION: pocaxis
 * @title:  PocAxis
 * @short_description: Axis gadget for #PocPlot
 * @see_also: #PocPlot #PocDataset
 *
 * Each dataset added to a plot requires an X and Y axis.  Several datasets may
 * share axes.  An axis controls the upper and lower bounds and the displayed
 * portion of the plot data.  If an axis is configured using a #GtkAdjustment
 * it may display a portion of its full range under control of the adjustment,
 * otherwise it shows the full range between its lower and upper bounds.
 *
 * Although axes contain drawing code, drawing always takes place under control
 * of the #PocPlot and on its canvas. Axes are vertical or horizontally
 * orientated and are drawn at the appropriate edge of the plot depending on
 * the associated dataset and plot surface. It is possible for the same axis to
 * be shared on more than one plot and may be orientated differently on each.
 * Axes are also responsible from drawing grid lines in the main plot area.
 */

typedef struct _PocAxisPrivate PocAxisPrivate;
struct _PocAxisPrivate
  {
    PocAxisMode		axis_mode;
    gdouble		lower_bound;
    gdouble		upper_bound;

    gdouble		major_interval;
    gboolean		auto_interval;
    guint		minor_divisions;

    GtkAdjustment	*adjustment;
    gulong		adj_changed_id;
    gulong		adj_value_changed_id;

    gfloat		tick_size;
    gfloat		label_size;
    PocLineStyle	major_grid;
    PocLineStyle	minor_grid;

    gchar		*legend;
    gfloat		legend_size;

    /* Bounds adjusted for the current mode */
    gdouble		lower_mode;
    gdouble		upper_mode;
    gdouble		minor_interval;
  };

G_DEFINE_TYPE_WITH_PRIVATE (PocAxis, poc_axis, G_TYPE_OBJECT)

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
 * poc_axis_new:
 *
 * Create a new #PocAxis
 *
 * Returns: (transfer full): New #PocAxis
 */
PocAxis *
poc_axis_new (void)
{
  return g_object_new (POC_TYPE_AXIS, NULL);
}

/* GObject {{{1 */

enum
  {
    PROP_0,
    PROP_AXIS_MODE,
    PROP_LOWER_BOUND,
    PROP_UPPER_BOUND,
    PROP_MAJOR_INTERVAL,
    PROP_AUTO_INTERVAL,
    PROP_MINOR_DIVISIONS,
    PROP_TICK_SIZE,
    PROP_LABEL_SIZE,
    PROP_MAJOR_GRID,
    PROP_MINOR_GRID,
    PROP_LEGEND,
    PROP_LEGEND_SIZE,
    PROP_ADJUSTMENT,

    N_PROPERTIES
  };
static GParamSpec *poc_axis_prop[N_PROPERTIES];

enum
  {
    UPDATE,
    N_SIGNAL
  };
static guint poc_axis_signals[N_SIGNAL];


static void poc_axis_dispose (GObject *object);
static void poc_axis_finalize (GObject *object);
static void poc_axis_get_property (GObject *object, guint param_id,
				     GValue *value, GParamSpec *pspec);
static void poc_axis_set_property (GObject *object, guint param_id,
				     const GValue *value, GParamSpec *pspec);
static void poc_axis_update_bounds (PocAxis *self);

static void
poc_axis_class_init (PocAxisClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);

  gobject_class->dispose = poc_axis_dispose;
  gobject_class->finalize = poc_axis_finalize;
  gobject_class->set_property = poc_axis_set_property;
  gobject_class->get_property = poc_axis_get_property;

  poc_axis_prop[PROP_AXIS_MODE] = g_param_spec_enum (
	"axis-mode",
	"Axis Mode", "Show axis as linear, octaves or decades",
	POC_TYPE_AXIS_MODE,
	POC_AXIS_LINEAR,
	G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);
  poc_axis_prop[PROP_LOWER_BOUND] = g_param_spec_double (
	"lower-bound",
	"Lower Bound", "Lower bound of range to plot on this axis",
	-1e6, 1e6, 0.0,
	G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);
  poc_axis_prop[PROP_UPPER_BOUND] = g_param_spec_double (
	"upper-bound",
	"Upper Bound", "Upper bound of range to plot on this axis",
	-1e6, 1e6, 1.0,
	G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  poc_axis_prop[PROP_MAJOR_INTERVAL] = g_param_spec_double (
	"major-interval",
	"Major Interval", "Major tick interval",
	0.0, 1e6, 10.0,
	G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);
  poc_axis_prop[PROP_AUTO_INTERVAL] = g_param_spec_boolean (
	"auto-interval",
	"Auto-Interval", "Automatically set major tick interval",
	TRUE,
	G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);
  poc_axis_prop[PROP_MINOR_DIVISIONS] = g_param_spec_uint (
	"minor-divisions",
	"Minor Divisions", "Minor tick interval on this axis",
	1, 100, 5,
	G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  poc_axis_prop[PROP_TICK_SIZE] = g_param_spec_float (
	"tick-size",
	"Tick Size", "Tick size on this axis",
	0.0f, 100.0f, 10.0f,
	G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);
  poc_axis_prop[PROP_LABEL_SIZE] = g_param_spec_float (
	"label-size",
	"Label Size", "Text size for tick label",
	2.0f, 100.0f, 10.0f,
	G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);
  poc_axis_prop[PROP_MAJOR_GRID] = g_param_spec_enum (
	"major-grid", "Major Grid",
	"Line style for major grid lines",
	POC_TYPE_LINE_STYLE, POC_LINE_STYLE_SOLID,
	G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);
  poc_axis_prop[PROP_MINOR_GRID] = g_param_spec_enum (
	"minor-grid", "Minor Grid",
	"Line style for minor grid lines",
	POC_TYPE_LINE_STYLE, POC_LINE_STYLE_DASH,
	G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);


  poc_axis_prop[PROP_LEGEND] = g_param_spec_string (
	"legend",
	"Legend", "Text for the axis legend.",
	NULL,
	G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);
  poc_axis_prop[PROP_LEGEND_SIZE] = g_param_spec_float (
	"legend-size",
	"Legend Size", "Text size for axis legend",
	2.0f, 100.0f, 14.0,
	G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  poc_axis_prop[PROP_ADJUSTMENT] = g_param_spec_object (
	"adjustment",
	"Adjustment", "Adjustment to scroll between bounds on axis",
	GTK_TYPE_ADJUSTMENT,
	G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (gobject_class, N_PROPERTIES, poc_axis_prop);

  poc_axis_signals[UPDATE] = g_signal_new (
	"update", G_TYPE_FROM_CLASS (class),
	G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
	0, NULL, NULL,
	g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
}


static void
poc_axis_init (PocAxis *self)
{
  PocAxisPrivate *priv = poc_axis_get_instance_private (self);

  priv->axis_mode = POC_AXIS_LINEAR;
  priv->lower_mode = priv->lower_bound = 0.0;
  priv->upper_mode = priv->upper_bound = 1.0;
  priv->major_interval = 10.0;
  priv->auto_interval = TRUE;
  priv->minor_divisions = 5;
  priv->tick_size = 10.0f;
  priv->label_size = 10.0f;
  priv->major_grid = POC_LINE_STYLE_SOLID;
  priv->minor_grid = POC_LINE_STYLE_DASH;
  priv->legend_size = 14.0f;
  poc_axis_update_bounds (self);
}

static void
poc_axis_dispose (GObject *object)
{
  PocAxis *self = (PocAxis *) object;
  PocAxisPrivate *priv = poc_axis_get_instance_private (self);

  g_clear_signal_handler (&priv->adj_changed_id, priv->adjustment);
  g_clear_signal_handler (&priv->adj_value_changed_id, priv->adjustment);
  g_clear_object (&priv->adjustment);

  G_OBJECT_CLASS (poc_axis_parent_class)->dispose (object);
}

static void
poc_axis_finalize (GObject *object)
{
  PocAxis *self = (PocAxis *) object;
  PocAxisPrivate *priv = poc_axis_get_instance_private (self);

  g_free (priv->legend);
  G_OBJECT_CLASS (poc_axis_parent_class)->finalize (object);
}

static void
poc_axis_set_property (GObject *object, guint prop_id,
			 const GValue *value, GParamSpec *pspec)
{
  PocAxis *self = POC_AXIS (object);

  switch (prop_id)
    {
    case PROP_AXIS_MODE:
      poc_axis_set_axis_mode (self, g_value_get_enum (value));
      break;
    case PROP_LOWER_BOUND:
      poc_axis_set_lower_bound (self, g_value_get_double (value));
      break;
    case PROP_UPPER_BOUND:
      poc_axis_set_upper_bound (self, g_value_get_double (value));
      break;

    case PROP_MAJOR_INTERVAL:
      poc_axis_set_major_interval (self, g_value_get_double (value));
      break;
    case PROP_AUTO_INTERVAL:
      poc_axis_set_auto_interval (self, g_value_get_boolean (value));
      break;
    case PROP_MINOR_DIVISIONS:
      poc_axis_set_minor_divisions (self, g_value_get_uint (value));
      break;

    case PROP_TICK_SIZE:
      poc_axis_set_tick_size (self, g_value_get_float (value));
      break;
    case PROP_LABEL_SIZE:
      poc_axis_set_label_size (self, g_value_get_float (value));
      break;
    case PROP_MAJOR_GRID:
      poc_axis_set_major_grid (self, g_value_get_enum (value));
      break;
    case PROP_MINOR_GRID:
      poc_axis_set_minor_grid (self, g_value_get_enum (value));
      break;

    case PROP_LEGEND:
      poc_axis_set_legend (self, g_value_get_string (value));
      break;
    case PROP_LEGEND_SIZE:
      poc_axis_set_legend_size (self, g_value_get_float (value));
      break;

    case PROP_ADJUSTMENT:
      poc_axis_set_adjustment (self, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      return;
    }
}

static void
poc_axis_get_property (GObject *object, guint prop_id,
			 GValue *value, GParamSpec *pspec)
{
  PocAxis *self = POC_AXIS (object);

  switch (prop_id)
    {
    case PROP_AXIS_MODE:
      g_value_set_enum (value, poc_axis_get_axis_mode (self));
      break;
    case PROP_LOWER_BOUND:
      g_value_set_double (value, poc_axis_get_lower_bound (self));
      break;
    case PROP_UPPER_BOUND:
      g_value_set_double (value, poc_axis_get_upper_bound (self));
      break;

    case PROP_MAJOR_INTERVAL:
      g_value_set_double (value, poc_axis_get_major_interval (self));
      break;
    case PROP_AUTO_INTERVAL:
      g_value_set_boolean (value, poc_axis_get_auto_interval (self));
      break;
    case PROP_MINOR_DIVISIONS:
      g_value_set_uint (value, poc_axis_get_minor_divisions (self));
      break;

    case PROP_TICK_SIZE:
      g_value_set_float (value, poc_axis_get_tick_size (self));
      break;
    case PROP_LABEL_SIZE:
      g_value_set_float (value, poc_axis_get_label_size (self));
      break;
    case PROP_MAJOR_GRID:
      g_value_set_enum (value, poc_axis_get_major_grid (self));
      break;
    case PROP_MINOR_GRID:
      g_value_set_enum (value, poc_axis_get_minor_grid (self));
      break;

    case PROP_LEGEND:
      g_value_set_string (value, poc_axis_get_legend (self));
      break;
    case PROP_LEGEND_SIZE:
      g_value_set_float (value, poc_axis_get_legend_size (self));
      break;

    case PROP_ADJUSTMENT:
      g_value_set_object (value, poc_axis_get_adjustment (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

/* Properties {{{1 */

/* legend position {{{2 */

/**
 * poc_axis_set_axis_mode:
 * @self: A #PocAxis
 * @axis_mode: A #PocAxisMode
 *
 * Show axis as linear, octaves or decades.
 */
void
poc_axis_set_axis_mode (PocAxis *self, PocAxisMode axis_mode)
{
  PocAxisPrivate *priv = poc_axis_get_instance_private (self);

  g_return_if_fail (POC_IS_AXIS (self));

  g_object_freeze_notify (G_OBJECT (self));
  priv->axis_mode = axis_mode;
  poc_axis_update_bounds (self);
  g_object_notify_by_pspec (G_OBJECT (self), poc_axis_prop[PROP_AXIS_MODE]);
  g_object_thaw_notify (G_OBJECT (self));
  poc_axis_notify_update (self);
}

/**
 * poc_axis_get_axis_mode:
 * @self: A #PocAxis
 *
 * Return the current axis mode.
 *
 * Returns: a #PocAxisMode
 */
PocAxisMode
poc_axis_get_axis_mode (PocAxis *self)
{
  PocAxisPrivate *priv = poc_axis_get_instance_private (self);

  g_return_val_if_fail (POC_IS_AXIS (self), POC_AXIS_LINEAR);

  return priv->axis_mode;
}

/* lower bound {{{2 */

/**
 * poc_axis_set_lower_bound:
 * @self: A #PocAxis
 * @bound: lower bound
 *
 * Set lower bound of range to plot on this axis.
 */
void
poc_axis_set_lower_bound (PocAxis *self, gdouble bound)
{
  PocAxisPrivate *priv = poc_axis_get_instance_private (self);

  g_return_if_fail (POC_IS_AXIS (self));

  g_object_freeze_notify (G_OBJECT (self));
  priv->lower_bound = bound;
  poc_axis_update_bounds (self);
  if (priv->adjustment != NULL)
    gtk_adjustment_set_lower (priv->adjustment, priv->lower_mode);
  g_object_notify_by_pspec (G_OBJECT (self), poc_axis_prop[PROP_LOWER_BOUND]);
  g_object_thaw_notify (G_OBJECT (self));
  poc_axis_notify_update (self);
}

/**
 * poc_axis_get_lower_bound:
 * @self: A #PocAxis
 *
 * Get the lower bound of the axis range.
 *
 * Returns: lower bound
 */
gdouble
poc_axis_get_lower_bound (PocAxis *self)
{
  PocAxisPrivate *priv = poc_axis_get_instance_private (self);

  g_return_val_if_fail (POC_IS_AXIS (self), 0.0);

  return priv->lower_bound;
}

/* upper bound {{{2 */

/**
 * poc_axis_set_upper_bound:
 * @self: A #PocAxis
 * @bound: upper bound
 *
 * Set upper bound of range to plot on this axis.
 */
void
poc_axis_set_upper_bound (PocAxis *self, gdouble bound)
{
  PocAxisPrivate *priv = poc_axis_get_instance_private (self);

  g_return_if_fail (POC_IS_AXIS (self));

  g_object_freeze_notify (G_OBJECT (self));
  priv->upper_bound = bound;
  poc_axis_update_bounds (self);
  if (priv->adjustment != NULL)
    gtk_adjustment_set_upper (priv->adjustment, priv->upper_mode);
  g_object_notify_by_pspec (G_OBJECT (self), poc_axis_prop[PROP_UPPER_BOUND]);
  g_object_thaw_notify (G_OBJECT (self));
  poc_axis_notify_update (self);
}

/**
 * poc_axis_get_upper_bound:
 * @self: A #PocAxis
 *
 * Get the upper bound of the axis range.
 *
 * Returns: upper bound
 */
gdouble
poc_axis_get_upper_bound (PocAxis *self)
{
  PocAxisPrivate *priv = poc_axis_get_instance_private (self);

  g_return_val_if_fail (POC_IS_AXIS (self), 0.0);

  return priv->upper_bound;
}

/* adjustment {{{2 */
static void
poc_axis_adj_value_changed (GtkAdjustment *adjustment, PocAxis *self)
{
  PocAxisPrivate *priv = poc_axis_get_instance_private (self);
  gdouble value, page_size;

  value = gtk_adjustment_get_value (adjustment);
  page_size = gtk_adjustment_get_page_size (adjustment);
  priv->lower_mode = value;
  priv->upper_mode = value + page_size;
  poc_axis_notify_update (self);
}

static void
poc_axis_adj_changed (GtkAdjustment *adjustment, PocAxis *self)
{
  PocAxisPrivate *priv = poc_axis_get_instance_private (self);
  gdouble lower, upper;

  lower = gtk_adjustment_get_lower (adjustment);
  upper = gtk_adjustment_get_upper (adjustment);
  switch (priv->axis_mode)
    {
    case POC_AXIS_LOG_OCTAVE:
      lower = exp2 (lower);
      upper = exp2 (upper);
      break;
    case POC_AXIS_LOG_DECADE:
      lower = exp10 (lower);
      upper = exp10 (upper);
      break;
    default:
      break;
    }
  priv->lower_bound = lower;
  priv->upper_bound = upper;
  g_object_notify_by_pspec (G_OBJECT (self), poc_axis_prop[PROP_LOWER_BOUND]);
  g_object_notify_by_pspec (G_OBJECT (self), poc_axis_prop[PROP_UPPER_BOUND]);
  poc_axis_adj_value_changed (adjustment, self);
}

/**
 * poc_axis_set_adjustment:
 * @self: A #PocAxis
 * @adjustment: A #GtkAdjustment
 *
 * Set an adjustment to scroll between bounds on axis.
 */
void
poc_axis_set_adjustment (PocAxis *self, GtkAdjustment *adjustment)
{
  PocAxisPrivate *priv = poc_axis_get_instance_private (self);
  gdouble page_size;

  g_return_if_fail (POC_IS_AXIS (self));
  g_return_if_fail (adjustment == NULL || GTK_IS_ADJUSTMENT (adjustment));

  g_clear_signal_handler (&priv->adj_changed_id, priv->adjustment);
  g_clear_signal_handler (&priv->adj_value_changed_id, priv->adjustment);
  g_set_object (&priv->adjustment, adjustment);
  if (priv->adjustment != NULL)
    {
      /* call poc_axis_update_bounds() to ensure _mode variables are correct */
      poc_axis_update_bounds (self);
      page_size = priv->upper_mode - priv->lower_mode;
      gtk_adjustment_configure (priv->adjustment,
				priv->lower_mode,
				priv->lower_mode, priv->upper_mode,
				page_size / 10.0, page_size / 2.0, page_size);
      priv->adj_changed_id = g_signal_connect_object (priv->adjustment,
				    "changed",
				    G_CALLBACK (poc_axis_adj_changed),
				    self, 0);
      priv->adj_value_changed_id = g_signal_connect_object (priv->adjustment,
				    "value-changed",
				    G_CALLBACK (poc_axis_adj_value_changed),
				    self, 0);
    }
  g_object_notify_by_pspec (G_OBJECT (self), poc_axis_prop[PROP_ADJUSTMENT]);
}

/**
 * poc_axis_get_adjustment:
 * @self: A #PocAxis
 *
 * Get the axis adjustment.
 *
 * Returns: (transfer none): A #GtkAdjustment
 */
GtkAdjustment *
poc_axis_get_adjustment (PocAxis *self)
{
  PocAxisPrivate *priv = poc_axis_get_instance_private (self);

  g_return_val_if_fail (POC_IS_AXIS (self), NULL);

  return priv->adjustment;
}

/* major interval {{{2 */

/**
 * poc_axis_set_major_interval:
 * @self: A #PocAxis
 * @interval: tick interval
 *
 * Set the major tick interval.
 */
void
poc_axis_set_major_interval (PocAxis *self, gdouble interval)
{
  PocAxisPrivate *priv = poc_axis_get_instance_private (self);

  g_return_if_fail (POC_IS_AXIS (self));

  g_object_freeze_notify (G_OBJECT (self));
  priv->major_interval = interval;
  priv->auto_interval = FALSE;
  poc_axis_update_bounds (self);
  g_object_notify_by_pspec (G_OBJECT (self), poc_axis_prop[PROP_MAJOR_INTERVAL]);
  g_object_notify_by_pspec (G_OBJECT (self), poc_axis_prop[PROP_AUTO_INTERVAL]);
  g_object_thaw_notify (G_OBJECT (self));
  poc_axis_notify_update (self);
}

/**
 * poc_axis_get_major_interval:
 * @self: A #PocAxis
 *
 * Get the major tick interval.
 *
 * Returns: interval
 */
gdouble
poc_axis_get_major_interval (PocAxis *self)
{
  PocAxisPrivate *priv = poc_axis_get_instance_private (self);

  g_return_val_if_fail (POC_IS_AXIS (self), 0.0);

  return priv->major_interval;
}

/* auto interval {{{2 */

/**
 * poc_axis_set_auto_interval:
 * @self: A #PocAxis
 * @enabled: enable auto calculation
 *
 * Automatically set major tick interval if @enabled is %TRUE.
 */
void
poc_axis_set_auto_interval (PocAxis *self, gboolean enabled)
{
  PocAxisPrivate *priv = poc_axis_get_instance_private (self);

  g_return_if_fail (POC_IS_AXIS (self));

  g_object_freeze_notify (G_OBJECT (self));
  priv->auto_interval = enabled;
  poc_axis_update_bounds (self);
  g_object_notify_by_pspec (G_OBJECT (self), poc_axis_prop[PROP_AUTO_INTERVAL]);
  g_object_thaw_notify (G_OBJECT (self));
  poc_axis_notify_update (self);
}

/**
 * poc_axis_get_auto_interval:
 * @self: A #PocAxis
 *
 * Get whether auto calculation of the major interval is enabled.
 *
 * Returns: %TRUE if enabled
 */
gboolean
poc_axis_get_auto_interval (PocAxis *self)
{
  PocAxisPrivate *priv = poc_axis_get_instance_private (self);

  g_return_val_if_fail (POC_IS_AXIS (self), 0.0);

  return priv->auto_interval;
}

/* minor divisions {{{2 */

/**
 * poc_axis_set_minor_divisions:
 * @self: A #PocAxis
 * @divisions: tick interval
 *
 * Minor tick interval on this axis.
 */
void
poc_axis_set_minor_divisions (PocAxis *self, guint divisions)
{
  PocAxisPrivate *priv = poc_axis_get_instance_private (self);

  g_return_if_fail (POC_IS_AXIS (self));

  g_object_freeze_notify (G_OBJECT (self));
  priv->minor_divisions = divisions;
  poc_axis_update_bounds (self);
  g_object_notify_by_pspec (G_OBJECT (self), poc_axis_prop[PROP_MAJOR_INTERVAL]);
  g_object_thaw_notify (G_OBJECT (self));
  poc_axis_notify_update (self);
}

/**
 * poc_axis_get_minor_divisions:
 * @self: A #PocAxis
 *
 * Get the minor tick interval.
 *
 * Returns: interval
 */
guint
poc_axis_get_minor_divisions (PocAxis *self)
{
  PocAxisPrivate *priv = poc_axis_get_instance_private (self);

  g_return_val_if_fail (POC_IS_AXIS (self), 0.0);

  return priv->minor_divisions;
}

/* tick size {{{1 */

/**
 * poc_axis_set_tick_size:
 * @self: A #PocAxis
 * @size: tick size
 *
 * Set the tick size on this axis.
 */
void
poc_axis_set_tick_size (PocAxis *self, gfloat size)
{
  PocAxisPrivate *priv = poc_axis_get_instance_private (self);

  g_return_if_fail (POC_IS_AXIS (self));

  priv->tick_size = size;
  g_object_notify_by_pspec (G_OBJECT (self), poc_axis_prop[PROP_TICK_SIZE]);
  poc_axis_notify_update (self);
}

/**
 * poc_axis_get_tick_size:
 * @self: A #PocAxis
 *
 * Get the tick size.
 *
 * Returns: tick size
 */
gfloat
poc_axis_get_tick_size (PocAxis *self)
{
  PocAxisPrivate *priv = poc_axis_get_instance_private (self);

  g_return_val_if_fail (POC_IS_AXIS (self), 0.0);

  return priv->tick_size;
}

/* label size {{{1 */

/**
 * poc_axis_set_label_size:
 * @self: A #PocAxis
 * @size: text size
 *
 * Set text size for tick label.
 */
void
poc_axis_set_label_size (PocAxis *self, gfloat size)
{
  PocAxisPrivate *priv = poc_axis_get_instance_private (self);

  g_return_if_fail (POC_IS_AXIS (self));

  priv->label_size = size;
  g_object_notify_by_pspec (G_OBJECT (self), poc_axis_prop[PROP_LABEL_SIZE]);
  poc_axis_notify_update (self);
}

/**
 * poc_axis_get_label_size:
 * @self: A #PocAxis
 *
 * Get text size for the tick label.
 *
 * Returns: size
 */
gfloat
poc_axis_get_label_size (PocAxis *self)
{
  PocAxisPrivate *priv = poc_axis_get_instance_private (self);

  g_return_val_if_fail (POC_IS_AXIS (self), 0.0);

  return priv->label_size;
}

/* legend {{{2 */

static gchar *
maybe_null (const gchar *str)
{
  return (str != NULL && *str != '\0') ? g_strdup (str) : NULL;
}

/**
 * poc_axis_set_legend:
 * @self: A #PocAxis
 * @legend: legend text
 *
 * Set the legend text for the axis.
 */
void
poc_axis_set_legend (PocAxis *self, const gchar *legend)
{
  PocAxisPrivate *priv = poc_axis_get_instance_private (self);

  g_return_if_fail (POC_IS_AXIS (self));

  g_free (priv->legend);
  priv->legend = maybe_null (legend);
  g_object_notify_by_pspec (G_OBJECT (self), poc_axis_prop[PROP_LEGEND]);
  poc_axis_notify_update (self);
}

/**
 * poc_axis_get_legend:
 * @self: A #PocAxis
 *
 * Get the legend text for the axis.
 *
 * Returns: (transfer none): legend text
 */
const gchar *
poc_axis_get_legend (PocAxis *self)
{
  PocAxisPrivate *priv = poc_axis_get_instance_private (self);

  g_return_val_if_fail (POC_IS_AXIS (self), NULL);

  return priv->legend;
}

/* legend size {{{1 */

/**
 * poc_axis_set_legend_size:
 * @self: A #PocAxis
 * @size: Size for legend text
 *
 * Set text size for the axis legend.
 */
void
poc_axis_set_legend_size (PocAxis *self, gfloat size)
{
  PocAxisPrivate *priv = poc_axis_get_instance_private (self);

  g_return_if_fail (POC_IS_AXIS (self));

  priv->legend_size = size;
  g_object_notify_by_pspec (G_OBJECT (self), poc_axis_prop[PROP_LEGEND_SIZE]);
  poc_axis_notify_update (self);
}

/**
 * poc_axis_get_legend_size:
 * @self: A #PocAxis
 *
 * Get text size for the axis legend.
 *
 * Returns: text size
 */
gfloat
poc_axis_get_legend_size (PocAxis *self)
{
  PocAxisPrivate *priv = poc_axis_get_instance_private (self);

  g_return_val_if_fail (POC_IS_AXIS (self), 0.0);

  return priv->legend_size;
}

/* "major_grid" {{{2 */

/**
 * poc_axis_set_major_grid:
 * @self: A #PocAxis
 * @major_grid:  Requested line style
 *
 * Set the line style for major grid lines.
 */
void
poc_axis_set_major_grid (PocAxis *self, PocLineStyle major_grid)
{
  PocAxisPrivate *priv = poc_axis_get_instance_private (self);

  g_return_if_fail (POC_IS_AXIS (self));

  if (priv->major_grid != major_grid)
    {
      priv->major_grid = major_grid;
      g_object_notify_by_pspec (G_OBJECT (self), poc_axis_prop[PROP_MAJOR_GRID]);
      poc_axis_notify_update (self);
    }
}

/**
 * poc_axis_get_major_grid:
 * @self: A #PocAxis
 *
 * Get the line style for major grid lines.
 *
 * Returns: a #PocLineStyle
 */
PocLineStyle
poc_axis_get_major_grid (PocAxis *self)
{
  PocAxisPrivate *priv = poc_axis_get_instance_private (self);

  g_return_val_if_fail (POC_IS_AXIS (self), POC_LINE_STYLE_SOLID);

  return priv->major_grid;
}

/* "minor_grid" {{{2 */

/**
 * poc_axis_set_minor_grid:
 * @self: A #PocAxis
 * @minor_grid: Requested line style
 *
 * Set the line style for minor grid lines.
 */
void
poc_axis_set_minor_grid (PocAxis *self, PocLineStyle minor_grid)
{
  PocAxisPrivate *priv = poc_axis_get_instance_private (self);

  g_return_if_fail (POC_IS_AXIS (self));

  if (priv->minor_grid != minor_grid)
    {
      priv->minor_grid = minor_grid;
      g_object_notify_by_pspec (G_OBJECT (self), poc_axis_prop[PROP_MINOR_GRID]);
      poc_axis_notify_update (self);
    }
}

/**
 * poc_axis_get_minor_grid:
 * @self: A #PocAxis
 *
 * Get the line style for minor grid lines.
 *
 * Returns: a #PocLineStyle
 */
PocLineStyle
poc_axis_get_minor_grid (PocAxis *self)
{
  PocAxisPrivate *priv = poc_axis_get_instance_private (self);

  g_return_val_if_fail (POC_IS_AXIS (self), POC_LINE_STYLE_DASH);

  return priv->minor_grid;
}

/* virtual/private methods {{{1 */

void
poc_axis_notify_update (PocAxis *self)
{
  g_return_if_fail (POC_IS_AXIS (self));
  g_signal_emit (self, poc_axis_signals[UPDATE], 0);
}

/* range of axes {{{1 */

/* XXX
  lower/upper_bound are fixed and configure the adjustment range and vice versa
  lower/upper_display are set from the position and page size of the adjustment
  lower/upper_mode are computed from lower/upper_display
 */

static void
poc_axis_update_bounds (PocAxis *self)
{
  PocAxisPrivate *priv = poc_axis_get_instance_private (self);
  gdouble temp, interval;

  if (priv->upper_bound < priv->lower_bound)
    {
      g_warning ("Lower axis bound less than upper bound, swapping");
      temp = priv->lower_bound;
      priv->lower_bound = priv->upper_bound;
      priv->upper_bound = temp;
      g_object_notify_by_pspec (G_OBJECT (self), poc_axis_prop[PROP_UPPER_BOUND]);
      g_object_notify_by_pspec (G_OBJECT (self), poc_axis_prop[PROP_LOWER_BOUND]);
    }

  switch (priv->axis_mode)
    {
    case POC_AXIS_LINEAR:
      priv->lower_mode = priv->lower_bound;
      priv->upper_mode = priv->upper_bound;
      break;
    case POC_AXIS_LOG_OCTAVE:
      priv->lower_mode = log2 (priv->lower_bound);
      priv->upper_mode = log2 (priv->upper_bound);
      break;
    case POC_AXIS_LOG_DECADE:
      priv->lower_mode = log10 (priv->lower_bound);
      priv->upper_mode = log10 (priv->upper_bound);
      break;
    }

  if (priv->auto_interval)
    {
      interval = priv->upper_mode - priv->lower_mode;
      if (interval >= 10000.0)
	priv->major_interval = 10000.0;
      else if (interval >= 1000.0)
	priv->major_interval = 1000.0;
      else if (interval >= 100.0)
	priv->major_interval = 100.0;
      else if (interval >= 10.0)
	priv->major_interval = 10.0;
      else
	priv->major_interval = 1.0;
      g_object_notify_by_pspec (G_OBJECT (self), poc_axis_prop[PROP_MAJOR_INTERVAL]);
    }
  if (priv->minor_divisions != 0)
    priv->minor_interval = priv->major_interval / priv->minor_divisions;
}

/* Always uses linear value */

/**
 * poc_axis_configure:
 * @self: A #PocAxis
 * @axis_mode: A #PocAxisMode
 * @lower_bound: axis lower bound
 * @upper_bound: axis upper bound
 *
 * Set axis parameters in a single call.
 */
void
poc_axis_configure (PocAxis *self, PocAxisMode axis_mode,
		    gdouble lower_bound, gdouble upper_bound)
{
  PocAxisPrivate *priv = poc_axis_get_instance_private (self);

  g_return_if_fail (POC_IS_AXIS (self));

  g_object_freeze_notify (G_OBJECT (self));

  priv->lower_bound = lower_bound;
  priv->upper_bound = upper_bound;
  priv->axis_mode = axis_mode;
  poc_axis_update_bounds (self);

  g_object_notify_by_pspec (G_OBJECT (self), poc_axis_prop[PROP_UPPER_BOUND]);
  g_object_notify_by_pspec (G_OBJECT (self), poc_axis_prop[PROP_LOWER_BOUND]);
  g_object_notify_by_pspec (G_OBJECT (self), poc_axis_prop[PROP_AXIS_MODE]);
  g_object_thaw_notify (G_OBJECT (self));
  poc_axis_notify_update (self);
}

/* Always uses linear value */

/**
 * poc_axis_get_range:
 * @self: A #PocAxis
 * @lower_bound: axis lower bound
 * @upper_bound: axis upper bound
 *
 * Get full axis range. The visible part of the axis may be less if scrolling.
 */
void
poc_axis_get_range (PocAxis *self, gdouble *lower_bound, gdouble *upper_bound)
{
  PocAxisPrivate *priv = poc_axis_get_instance_private (self);

  g_return_if_fail (POC_IS_AXIS (self));

  *lower_bound = priv->lower_bound;
  *upper_bound = priv->upper_bound;
}

/* Always uses linear value - will need antilog of adjustment page position */

/**
 * poc_axis_get_display_range:
 * @self: A #PocAxis
 * @lower_bound: axis lower bound
 * @upper_bound: axis upper bound
 *
 * Get displayed axis range. This may be less than the full range if scrolling.
 */
void
poc_axis_get_display_range (PocAxis *self, gdouble *lower_bound, gdouble *upper_bound)
{
  PocAxisPrivate *priv = poc_axis_get_instance_private (self);
  gdouble value, page_size;

  g_return_if_fail (POC_IS_AXIS (self));

  if (priv->adjustment != NULL)
    {
      value = gtk_adjustment_get_value (priv->adjustment);
      page_size = gtk_adjustment_get_page_size (priv->adjustment);
      switch (priv->axis_mode)
	{
	case POC_AXIS_LINEAR:
	  *lower_bound = value;
	  *upper_bound = value + page_size;
	  break;
	case POC_AXIS_LOG_OCTAVE:
	  *lower_bound = exp2 (value);
	  *upper_bound = exp2 (value + page_size);
	  break;
	case POC_AXIS_LOG_DECADE:
	  *lower_bound = exp10 (value);
	  *upper_bound = exp10 (value + page_size);
	  break;
	}
    }
  else
    poc_axis_get_range (self, lower_bound, upper_bound);
}

/* drawing {{{1 */

/**
 * poc_axis_project:
 * @self: A #PocAxis
 * @value: value to project
 * @norm: normalisation value
 *
 * Project a value from the dataset to pixel based position.  The value is
 * interpreted according to the axis mode.  This function is intended for use
 * in drawing code in subclasses of #PocDataset
 *
 * Returns: projected value
 */
double
poc_axis_project (PocAxis *self, gdouble value, gint norm)
{
  PocAxisPrivate *priv = poc_axis_get_instance_private (self);

  g_assert (POC_IS_AXIS (self));

  switch (priv->axis_mode)
    {
    case POC_AXIS_LINEAR:
      break;
    case POC_AXIS_LOG_OCTAVE:
      value = log2 (value);
      break;
    case POC_AXIS_LOG_DECADE:
      value = log10 (value);
      break;
    }
  return poc_axis_linear_project (self, value, norm);
}

/**
 * poc_axis_linear_project:
 * @self: A #PocAxis
 * @value: value to project
 * @norm: normalisation value
 *
 * Project a value from the dataset to pixel based position.  The value is
 * projected using linear interpolation between the axis limits.  This function
 * is intended for use in drawing code in subclasses of #PocDataset
 *
 * Returns: projected value
 */
double
poc_axis_linear_project (PocAxis *self, gdouble value, gint norm)
{
  PocAxisPrivate *priv = poc_axis_get_instance_private (self);
  double scale = (norm < 0 ? -norm : norm) - 1.0;

  value = ((value - priv->lower_mode) / (priv->upper_mode - priv->lower_mode));
  if (norm < 0)
    return (1.0 - value) * scale;
  else
    return value * scale;
}

/* Draw grid */
static inline void
poc_axis_draw_grid_line (PocAxis *self, cairo_t *cr, GtkOrientation orientation,
			 guint width, guint height, double xy)
{
  switch (orientation)
    {
    case GTK_ORIENTATION_HORIZONTAL:
      cairo_move_to (cr, floor (poc_axis_linear_project (self, xy, width)) + 0.5, 0.5);
      cairo_rel_line_to (cr, 0, height - 0.5);
      break;
    case GTK_ORIENTATION_VERTICAL:
      cairo_move_to (cr, 0.5, floor (poc_axis_linear_project (self, xy, -height)) + 0.5);
      cairo_rel_line_to (cr, width - 0.5, 0);
      break;
    }
}

/**
 * poc_axis_draw_grid:
 * @self: A #PocAxis
 * @cr: A #cairo_t
 * @orientation: A #GtkOrientation
 * @width: Plot area width
 * @height: Plot area height
 * @style: A #GtkStyleContext
 *
 * Draw plot grid lines in main plot area.  Used by #PocPlot.
 */
void
poc_axis_draw_grid (PocAxis *self, cairo_t *cr, GtkOrientation orientation,
		    guint width, guint height, GtkStyleContext *style)
{
  PocAxisPrivate *priv = poc_axis_get_instance_private (self);
  gdouble xy, mxy;
  gdouble lower_floor;
  const double *dashes;
  int num_dashes;
  const GdkRGBA *major_stroke, *minor_stroke;
  GdkRGBA rgba;
  GtkStateFlags state;

  state = gtk_style_context_get_state (style);

  gtk_style_context_save (style);
  gtk_style_context_add_class (style, "grid");
  gtk_style_context_get_color (style, state, &rgba);
  major_stroke = &rgba;
  minor_stroke = &rgba;
  gtk_style_context_restore (style);


  lower_floor = floor (priv->lower_mode / priv->major_interval) * priv->major_interval;

  cairo_new_path (cr);

  /* Draw the major grid */
  for (xy = lower_floor;
       xy <= ceil (priv->upper_mode);
       xy += priv->major_interval)
    poc_axis_draw_grid_line (self, cr, orientation, width, height, xy);
  cairo_set_line_width (cr, 1.0);
  dashes = poc_line_style_get_dashes (priv->major_grid, &num_dashes);
  cairo_set_dash (cr, dashes, num_dashes, 0.0);
  gdk_cairo_set_source_rgba (cr, major_stroke);
  cairo_stroke (cr);

  /* Draw the minor grid */
  if (priv->minor_divisions != 0)
    {
      for (xy = lower_floor; xy <= priv->upper_mode; xy += priv->major_interval)
	if (priv->axis_mode == POC_AXIS_LOG_DECADE)
	  for (mxy = 2.0; mxy < 10.0; mxy += 1.0)
	    poc_axis_draw_grid_line (self, cr, orientation, width, height,
				     xy + log10 (mxy));
	else
	  for (mxy = priv->minor_interval;
	       mxy < priv->major_interval;
	       mxy += priv->minor_interval)
	    poc_axis_draw_grid_line (self, cr, orientation, width, height,
				     xy + mxy);
      cairo_set_line_width (cr, 0.5);
      dashes = poc_line_style_get_dashes (priv->minor_grid, &num_dashes);
      cairo_set_dash (cr, dashes, num_dashes, 0.0);
      gdk_cairo_set_source_rgba (cr, minor_stroke);
      cairo_stroke (cr);
    }
}

#define EXTRA	2.0

static inline void
poc_axis_draw_tick (PocAxis *self, cairo_t *cr,
		    GtkOrientation orientation, GtkPackType pack,
		    guint width, guint height, double xy, double size)
{
  double pos;

  switch (orientation)
    {
    case GTK_ORIENTATION_HORIZONTAL:
      pos = poc_axis_linear_project (self, xy, width);
      if (pos > width)
	return;
      cairo_move_to (cr, floor (pos) + 0.5, pack == GTK_PACK_START ? 0.5 : height - 0.5);
      cairo_rel_line_to (cr, 0, pack == GTK_PACK_START ? size : -size);
      break;
    case GTK_ORIENTATION_VERTICAL:
      pos = poc_axis_linear_project (self, xy, -height);
      if (pos > height)
	return;
      cairo_move_to (cr, pack == GTK_PACK_START ? width - 0.5 : 0.5, floor (pos) + 0.5);
      cairo_rel_line_to (cr, pack == GTK_PACK_START ? -size : size, 0);
      break;
    }
}

static inline void
poc_axis_label_tick (PocAxis *self, cairo_t *cr,
		     GtkOrientation orientation, GtkPackType pack,
		     guint width, guint height,
		     const gchar *label, double xy, double size)
{
  cairo_text_extents_t extents;
  double x, y;

  cairo_text_extents (cr, label, &extents);
  switch (orientation)
    {
    case GTK_ORIENTATION_HORIZONTAL:
      x = poc_axis_linear_project (self, xy, width);
      if (x >= width)
	return;
      x -= extents.width / 2;
      if (pack == GTK_PACK_START)
	y = size - extents.y_bearing + EXTRA;
      else
	y = height - 1 - size - EXTRA;
      if (x < 0)
	x = 0;
      else if (x >= width - extents.width)
	x = width - 1 - extents.width;
      break;
    case GTK_ORIENTATION_VERTICAL:
      y = poc_axis_linear_project (self, xy, -height);
      if (y >= height)
	return;
      if (pack == GTK_PACK_START)
	x = width - 1 - (size + extents.width) - EXTRA;
      else
	x = size + EXTRA;
      y -= extents.y_bearing / 2;
      if (y < -extents.y_bearing)
	y = -extents.y_bearing;
      else if (y >= height)
	y = height - 1;
      break;
    default:
      return;
    }
  cairo_move_to (cr, x, y);
  cairo_show_text (cr, label);
}

/**
 * poc_axis_draw_axis:
 * @self: A #PocAxis
 * @cr: A #cairo_t
 * @orientation: A #GtkOrientation
 * @pack: A #GtkPackType
 * @width: Plot area width
 * @height: Plot area height
 * @style: A #GtkStyleContext
 *
 * Draw plot grid lines in main plot area.  Used by #PocPlot.
 */
void
poc_axis_draw_axis (PocAxis *self, cairo_t *cr,
		    GtkOrientation orientation, GtkPackType pack,
		    guint width, guint height, GtkStyleContext *style)
{
  PocAxisPrivate *priv = poc_axis_get_instance_private (self);
  gdouble xy, mxy, axy;
  cairo_text_extents_t extents;
  gchar buffer[128];
  gfloat minor, ends;
  gdouble lower_floor;
  const GdkRGBA *major_stroke, *minor_stroke, *text_fill;
  GdkRGBA rgba;
  GtkStateFlags state;

  state = gtk_style_context_get_state (style);

  gtk_style_context_save (style);
  gtk_style_context_add_class (style, "grid");
  gtk_style_context_get_color (style, state, &rgba);
  major_stroke = &rgba;
  minor_stroke = &rgba;
  gtk_style_context_restore (style);

  lower_floor = floor (priv->lower_mode / priv->major_interval) * priv->major_interval;

  cairo_new_path (cr);

  /* Axis always stroked with a solid line */
  cairo_set_dash (cr, NULL, 0, 0.0);

  ends = priv->label_size;
  if (priv->tick_size > 0.0f)
    {
      ends += priv->tick_size + (float) EXTRA;

      /* frame and major ticks */
      switch (orientation)
	{
	case GTK_ORIENTATION_HORIZONTAL:
	  cairo_move_to (cr, 0, pack == GTK_PACK_START ? 0.5 : height - 0.5);
	  cairo_rel_line_to (cr, width - 1, 0);
	  break;
	case GTK_ORIENTATION_VERTICAL:
	  cairo_move_to (cr, pack == GTK_PACK_START ? width - 0.5 : 0.5, 0);
	  cairo_rel_line_to (cr, 0, height - 1);
	  break;
	}
      for (xy = lower_floor;
	   xy <= ceil (priv->upper_mode);
	   xy += priv->major_interval)
	poc_axis_draw_tick (self, cr, orientation, pack, width, height,
			    xy, priv->tick_size);
      cairo_set_line_width (cr, 1.0);
      gdk_cairo_set_source_rgba (cr, major_stroke);
      cairo_stroke (cr);

      /* minor ticks */
      if (priv->minor_divisions != 0)
	{
	  minor = priv->tick_size * 0.6f;
	  for (xy = lower_floor; xy <= priv->upper_mode; xy += priv->major_interval)
	    if (priv->axis_mode == POC_AXIS_LOG_DECADE)
	      for (mxy = 2.0; mxy < 10.0; mxy += 1.0)
		poc_axis_draw_tick (self, cr, orientation, pack, width, height,
				    xy + log10 (mxy), minor);
	    else
	      for (mxy = priv->minor_interval;
		   mxy < priv->major_interval;
		   mxy += priv->minor_interval)
		poc_axis_draw_tick (self, cr, orientation, pack, width, height,
				    xy + mxy, minor);
	  cairo_set_line_width (cr, 0.5);
	  gdk_cairo_set_source_rgba (cr, minor_stroke);
	  cairo_stroke (cr);
	}
    }

  /* Annotate major ticks */
  cairo_set_font_size (cr, priv->label_size);
  gdk_cairo_set_source_rgba (cr, major_stroke);
  for (xy = lower_floor; xy <= priv->upper_mode; xy += priv->major_interval)
    {
      axy = (priv->axis_mode == POC_AXIS_LOG_DECADE) ? exp10 (xy) : xy;
      g_snprintf (buffer, sizeof buffer, "%g", axy);
      poc_axis_label_tick (self, cr, orientation, pack, width, height,
			   buffer, xy, priv->tick_size);
    }

  /* Text */
  gtk_style_context_get_color (style, state, &rgba);
  text_fill = &rgba;
  gdk_cairo_set_source_rgba (cr, text_fill);

  /* Legend */
  if (priv->legend != NULL)
    {
      cairo_set_font_size (cr, priv->legend_size);
      cairo_text_extents (cr, priv->legend, &extents);
      cairo_save (cr);
      switch (orientation)
	{
	case GTK_ORIENTATION_HORIZONTAL:
	  if (pack == GTK_PACK_START)
	    cairo_move_to (cr, (width - extents.width) / 2,
			       ends - extents.y_bearing);
	  else
	    cairo_move_to (cr, (width - extents.width) / 2,
			       -extents.y_bearing);
	  break;
	case GTK_ORIENTATION_VERTICAL:
	  if (pack == GTK_PACK_START)
	    {
	      cairo_move_to (cr, width - (ends - extents.y_bearing),
				 (height - extents.width) / 2);
	      cairo_rotate (cr, G_PI / 2.0);
	    }
	  else
	    {
	      cairo_move_to (cr, ends - extents.y_bearing,
				 (height + extents.width) / 2);
	      cairo_rotate (cr, -G_PI / 2.0);
	    }
	  break;
	}
      cairo_show_text (cr, priv->legend);
      cairo_restore (cr);
    }
}

/**
 * poc_axis_size:
 * @self: A #PocAxis
 *
 * Compute the width or height of the axis on the plot canvas.
 * Used by #PocPlot.
 */
gdouble
poc_axis_size (PocAxis *self)
{
  PocAxisPrivate *priv = poc_axis_get_instance_private (self);
  gdouble size;

  g_return_val_if_fail (POC_IS_AXIS (self), 0.0);

  size = priv->label_size;
  if (priv->tick_size > 0.0f)
    size += priv->tick_size + (float) EXTRA;
  if (priv->legend != NULL)
    size += priv->legend_size + (float) EXTRA;
  return size;
}
