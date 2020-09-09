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
#ifndef _pocaxis_h
#define _pocaxis_h

#if !defined(__poc_h_inside__) && !defined(__poc_compile__)
# error "only <poc.h> can be included directly"
#endif

#include <gtk/gtk.h>
#include "poctypes.h"

G_BEGIN_DECLS

#define POC_TYPE_AXIS			poc_axis_get_type ()
G_DECLARE_DERIVABLE_TYPE (PocAxis, poc_axis, POC, AXIS, GObject)

struct _PocAxisClass
{
  /*< private >*/
  GObjectClass    parent_class;

  gpointer dummy1;
  gpointer dummy2;
  gpointer dummy3;
  gpointer dummy4;
};

PocAxis *	poc_axis_new (void);

void		poc_axis_set_axis_mode (PocAxis *self, PocAxisMode axis_mode);
PocAxisMode	poc_axis_get_axis_mode (PocAxis *self);
void		poc_axis_set_lower_bound (PocAxis *self, gdouble bound);
gdouble		poc_axis_get_lower_bound (PocAxis *self);
void		poc_axis_set_upper_bound (PocAxis *self, gdouble bound);
gdouble		poc_axis_get_upper_bound (PocAxis *self);
void		poc_axis_set_adjustment (PocAxis *self, GtkAdjustment *adjustment);
GtkAdjustment *	poc_axis_get_adjustment (PocAxis *self);
void		poc_axis_set_major_interval (PocAxis *self, gdouble interval);
gdouble		poc_axis_get_major_interval (PocAxis *self);
void		poc_axis_set_auto_interval (PocAxis *self, gboolean enabled);
gboolean	poc_axis_get_auto_interval (PocAxis *self);
void		poc_axis_set_minor_divisions (PocAxis *self, guint divisions);
guint		poc_axis_get_minor_divisions (PocAxis *self);
void		poc_axis_set_tick_size (PocAxis *self, gfloat size);
gfloat		poc_axis_get_tick_size (PocAxis *self);
void		poc_axis_set_label_size (PocAxis *self, gfloat size);
gfloat		poc_axis_get_label_size (PocAxis *self);
void		poc_axis_set_major_grid (PocAxis *self, PocLineStyle major_grid);
PocLineStyle	poc_axis_get_major_grid (PocAxis *self);
void		poc_axis_set_minor_grid (PocAxis *self, PocLineStyle minor_grid);
PocLineStyle	poc_axis_get_minor_grid (PocAxis *self);
void		poc_axis_set_legend (PocAxis *self, const gchar *legend);
const gchar *	poc_axis_get_legend (PocAxis *self);
void		poc_axis_set_legend_size (PocAxis *self, gfloat size);
gfloat		poc_axis_get_legend_size (PocAxis *self);

void		poc_axis_notify_update (PocAxis *self);
void		poc_axis_draw_axis (PocAxis *self, cairo_t *cr,
				    GtkOrientation orientation,
				    GtkPackType pack,
				    guint width, guint height,
				    GtkStyleContext *style);
void		poc_axis_draw_grid (PocAxis *self, cairo_t *cr,
				    GtkOrientation orientation,
				    guint width, guint height,
				    GtkStyleContext *style);
gdouble		poc_axis_size (PocAxis *self);

/* convenience access to bounds */
void		poc_axis_configure (PocAxis *self,
				    PocAxisMode axis_mode,
				    gdouble lower_bound,
				    gdouble upper_bound);

void		poc_axis_get_range (PocAxis *self,
				    gdouble *lower_bound,
				    gdouble *upper_bound);
void		poc_axis_get_display_range (PocAxis *self,
					    gdouble *lower_bound,
					    gdouble *upper_bound);

double		poc_axis_linear_project (PocAxis *self, gdouble value, gint norm);
double		poc_axis_project (PocAxis *self, gdouble value, gint norm);

G_END_DECLS

#endif
